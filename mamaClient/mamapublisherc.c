/* $Id$
*
* OpenMAMA: The open middleware agnostic messaging API
* Copyright (C) 2011 NYSE Technologies, Inc.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
* 02110-1301 USA
*/

#include "wombat/port.h"

#include "mama/mama.h"
#include "string.h"

#define MAX_PUBLISHERS 2500

#if _MSC_VER > 1500
#define fileno _fileno
#define isatty _isatty
#endif

static mamaTransport    gTransport = NULL;
static mamaTimer        gTimer = NULL;
static mamaSubscription gSubscription = NULL;
static mamaPublisher    gPublisher = NULL;

static mamaPublisher*    gPublisherList = NULL;

static const char**      gSymbolList = NULL;
static int               gNumSymbols = 0;

static mamaMsgCallbacks gInboundCb;

static mamaInbox           gInbox;
static mamaSource       gSubscriptionSource = NULL;

static mamaDictionary   gDictionary;

static mamaBridge       gMamaBridge = NULL;
static mamaQueue        gMamaDefaultQueue = NULL;
static const char*      gMiddleware = "wmw";

static const char *     gOutBoundTopic  = "MAMA_TOPIC";

static const char *     gServiceSub = "DEV_OMM_INT_1";
static const char *     gServicePub = "DEV_OMM_INT_1";
static const char *     gSubSymbol = "";
static const char *     gTransportName  = "pub";
static double           gInterval = 0.5;
static double               gPubDelay = 3.0; /* delay before starting publisher*/
static mamaTimer           gPubDelayTimer;
static int              gQuietLevel = 0;
static int                   gRePub = 0;

static const char *     gFilename = NULL;

// Default to sending continuous live messages
static int              gStatusLiveCount = 1;
static int              gStatusStaleCount = 0;
static int              gStatusIsLive = -1;  /* TRUE */
static int              gStatusIndex = 0;

static int               gDictFromFile         = 0;
static const char*       gDictFile           = NULL;
static int               gBuildDataDict      = 1;

static int                 gShutdownTime         = 0;


static const char *     gUsageString[]  =
{
   " This sample application demonstrates how to publish mama messages, and",
   " respond to requests from a clients inbox.",
   "",
   " It accepts the following command line arguments:",
   "      [-pub topic]         The topic on which to send messages. Default is",
   "                         is \"MAMA_TOPIC\".",
   "      [-i interval]      The interval between messages .Default in  0.5.",
   "      [-tport name]      The transport parameters to be used from",
   "                         mama.properties. Default is pub",
   "      [-m middleware]    The middleware to use [wmw/lbm/tibrv].",
   "                         Default is wmw.",
   "      [-q]               Quiet mode. Suppress output.",
   "      [-v]               Increase verbosity. Can be passed multiple times",
   "      [-Ss]              RMDS subscribe source",
   "      [-Sp]              RMDS publish source",
   "      [-sub]             symbol to subscribe to",
   "      [-f filename]      list of symbols to publish",
   "      [-R]               Republish message received by subscribe (remove mama FIDs below 100)",
   "      [-status <l> <s> ] Ratio of message statuses, e.g. -status 5 1 means send five live,"
   "                         then one stale message, then five live then one stale, ad inf.",
   NULL
};

static void parseCommandLine(int argc, const char **argv);
static void initializeMama();
static void createIntervalTimer();
static void createSubscription();

static void MAMACALLTYPE timerCallback       (mamaTimer timer, void *closure);
static void MAMACALLTYPE subscriptionOnCreate     (mamaSubscription subscription, void *closure);

static void MAMACALLTYPE pubDelayTimerCallback (mamaTimer timer, void *closure);
static void createPubDelayTimer();

static void readSymbolsFromFile();

static void MAMACALLTYPE subscriptionOnError(mamaSubscription subscription, mama_status status,
                                             void* platformError, const char *subject, void *closure);

static void MAMACALLTYPE subscriptionOnMsg(mamaSubscription subscription, mamaMsg msg, void *closure,
                                           void *itemClosure);

static void publishMessage(mamaMsg request);
static void createPublisher(void);
static void usage(int exitStatus);
static void republish(mamaMsg msg);
static void createInbox();

void MAMACALLTYPE inboxOnMessage(mamaMsg msg, void *closure);

void displayAllFields (mamaMsg msg, mamaSubscription subscription, int indentLevel);

void MAMACALLTYPE displayCb(const mamaMsg msg, const mamaMsgField  field, void* closure);

// dictionary stuff
static void MAMACALLTYPE timeoutCb(mamaDictionary dict, void *closure);
static void MAMACALLTYPE errorCb(mamaDictionary dict, const char *errMsg, void *closure);
static void MAMACALLTYPE completeCb(mamaDictionary dict, void *closure);

static void buildDataDictionary (void);
static mamaDictionary    gDictionary;
static const char*       gDictSymbolNamespace  = "WOMBAT";
static mamaSource        gDictSource         = NULL;
static int               gDictionaryComplete = 0;
static const char*       gDictTport          = NULL;
static mamaTransport     gDictTransport      = NULL;


static void mamashutdown(void);


static void MAMACALLTYPE
    shutdownTimerCallback(mamaTimer timer, void *closure)
{
    /* Stop dispatching messages. */
    mama_stop(gMamaBridge);
}


mamaTimer shutdownTimer = NULL;


int main(int argc, const char **argv)
{

    mamaTimer shutdownTimer = NULL;

   parseCommandLine (argc, argv);

   gPublisherList = (mamaPublisher *) calloc(MAX_PUBLISHERS, sizeof (mamaPublisher));

   gSymbolList = (const char **) calloc(MAX_PUBLISHERS, sizeof (char*));

   if (gFilename != 0)
   {
      readSymbolsFromFile();
   }


   initializeMama();
   buildDataDictionary();
   // dump dictionary to file
   // mamaDictionary_writeToFile(gDictionary, "Dict.txt");

   //createIntervalTimer();
   /* Create the shutdown timer. */
   if (gShutdownTime > 0)
   {
       mamaTimer_create(&shutdownTimer, gMamaDefaultQueue, shutdownTimerCallback, gShutdownTime, NULL);
   }

   createPubDelayTimer();
   createSubscription();

   createPublisher();

   mama_start(gMamaBridge);

   /* Destroy the shutdown timer if it was created. */
   if (shutdownTimer != NULL)
   {
       mamaTimer_destroy(shutdownTimer);
   }

   mamashutdown ();
   return 0;
}

static void MAMACALLTYPE
transportCb (mamaTransport      tport,
             mamaTransportEvent ev,
             short              cause,
             const void*        platformInfo,
             void*              closure)
{
    printf ("Transport callback: %s\n", mamaTransportEvent_toString (ev));
}

void initializeMama()
{
   mama_status status;

   status = mama_loadBridge(&gMamaBridge, gMiddleware);
   if (status != MAMA_STATUS_OK)
   {
      printf("Error loading bridge: %s\n",
         mamaStatus_stringForStatus (status));
      exit (status);
   }

   status = mama_open();

   if (status != MAMA_STATUS_OK)
   {
      printf("Error initializing mama: %s\n",
         mamaStatus_stringForStatus (status));
      exit(status);
   }

   /*Use the default internal event queue for all subscriptions and timers*/
   mama_getDefaultEventQueue (gMamaBridge, &gMamaDefaultQueue);

   status = mamaTransport_allocate (&gTransport);

   if (status != MAMA_STATUS_OK)
   {
      printf("Error allocating transport: %s\n", mamaStatus_stringForStatus(status));
      exit(status);
   }

   status = mamaTransport_create(gTransport, gTransportName, gMamaBridge);

   if (status != MAMA_STATUS_OK)
   {
      printf("Error creating transport: %s\n",
         mamaStatus_stringForStatus (status));
      exit(status);
   }

   // dictionary
   gDictTransport = gTransport;

   /*The mamaSource used for creating the dictionary*/
   if (MAMA_STATUS_OK != (status = mamaSource_create(&gDictSource)))
   {
      fprintf (stderr,
         "Failed to create dictionary mamaSource STATUS: %d %s\n",
         status, mamaStatus_stringForStatus (status));
      exit(1);
   }

   mamaSource_setId(gDictSource, "Dictionary_Source");
   mamaSource_setTransport(gDictSource, gDictTransport);
   mamaSource_setSymbolNamespace(gDictSource, gDictSymbolNamespace);

   /*Register interest in transport related events*/
   status = mamaTransport_setTransportCallback(gTransport, transportCb, NULL);
}

static void MAMACALLTYPE publisherOnCreate(mamaPublisher publisher, void* closure)
{
    printf("Publisher is created\n");
}

static void MAMACALLTYPE publisherOnDestroy(mamaPublisher publisher, void* closure)
{
    printf("Publisher is destroyed\n");
}

static void MAMACALLTYPE publisherOnError(mamaPublisher publisher,
    mama_status   status,
    const char*   info,
    void*         closure)
{
    printf("An error in the Publisher is raised [%d](%s)\n", status, info);
}


static void createPublisher()
{
   mama_status status;
   mamaPublisherCallbacks* callbacks = NULL;

   int i;
   createInbox();

   mamaPublisherCallbacks_allocate(&callbacks);

   callbacks->onCreate = publisherOnCreate;
   callbacks->onError = publisherOnError;
   callbacks->onDestroy = publisherOnDestroy;

   if (gNumSymbols > 0)
   {
      // create array of publishers
      for (i = 0; i < gNumSymbols; i++)
      {
         status = mamaPublisher_createWithCallbacks(&gPublisherList[i], gTransport,
             gMamaDefaultQueue,
             gSymbolList[i],
             gServicePub,   /* Not needed for basic publishers */
             NULL,         /* Not needed for basic publishers */
             callbacks,
             NULL);
      }
   }
   else
   {
      status = mamaPublisher_createWithCallbacks(&gPublisher, gTransport,
          gMamaDefaultQueue,
          gOutBoundTopic,
          gServicePub,   /* Not needed for basic publishers */
          NULL,         /* Not needed for basic publishers */
          callbacks,
          NULL);
   }

   mamaPublisherCallbacks_deallocate(callbacks);

   if (status != MAMA_STATUS_OK)
   {
      printf("Error creating publisher: %s\n",
         mamaStatus_stringForStatus(status));
      exit(status);
   }
}

// Cycle through the message status sequence according to the "-status" command-line option
static mamaMsgStatus getNextMsgStatus()
{
   // Sort out the simple cases where messages are always live or always stale
   if (0 == gStatusStaleCount)
   {
      // Stale count is zero, so we are always live
      gStatusIsLive = -1;  /* TRUE */
   }
   else if (0 == gStatusLiveCount)
   {
      // Live count is zero, so we are always stale
      gStatusIsLive = 0;  /* FALSE */
   }
   else
   {
      // Otherwise, we are oscillating between the states
      ++gStatusIndex;
      if (gStatusIsLive && (gStatusIndex >= gStatusLiveCount))
      {
         printf("status is now MAMA_MSG_STATUS_POSSIBLY_STALE\n");
         gStatusIsLive = 0;  /* FALSE */
         gStatusIndex = 0;
      }
      else if (!gStatusIsLive && (gStatusIndex >= gStatusStaleCount))
      {
         printf("status is now MAMA_MSG_STATUS_OK\n");
         gStatusIsLive = -1;  /* TRUE */
         gStatusIndex = 0;
      }
   }

   return gStatusIsLive ? MAMA_MSG_STATUS_OK : MAMA_MSG_STATUS_POSSIBLY_STALE;
}

// need to iterate fields to remove FIDs below 100 as they are not in the RMDS dictionary
static void republish(mamaMsg msg)
{
   static mamaMsg newMsg;
   mama_status status = MAMA_STATUS_OK;
   //mamaPrice newPrice;
   mamaMsgIterator iterator = NULL;
   mamaMsgField field = NULL;
   mamaMsgStatus msgStatus = MAMA_MSG_STATUS_UNKNOWN;
   mamaFieldType fieldType = MAMA_FIELD_TYPE_UNKNOWN;
   const char* fieldName = NULL;
   uint16_t fid = 0;

   mamaMsg_create(&newMsg);
   mamaMsgIterator_create(&iterator, gDictionary);
   if (MAMA_STATUS_OK != (status = mamaMsgIterator_associate(iterator, msg)))
   {
      fprintf (stderr, "Could not associate iterator "
         "with message. [%s]\n", mamaStatus_stringForStatus(status));
   }
   else
   {
      while ((field = mamaMsgIterator_next(iterator)) != NULL)
      {
         // build new message - skip fids < 100
         mamaMsgField_getFid(field, &fid);
         if (fid < 100)
            continue;

         // get name
         mamaMsgField_getName(field, &fieldName);

         // get field type
         mamaMsgField_getType(field, &fieldType);
         switch(fieldType)
         {
         case MAMA_FIELD_TYPE_BOOL:
            {
               mama_bool_t result;
               mamaMsgField_getBool(field, &result);
               mamaMsg_addBool(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_CHAR:
            {
               char result;
               mamaMsgField_getChar(field, &result);
               mamaMsg_addChar(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_I8:
            {
               int8_t result;
               mamaMsgField_getI8(field, &result);
               mamaMsg_addI8(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_U8:
            {
               uint8_t result;
               mamaMsgField_getU8(field, &result);
               mamaMsg_addU8(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_I16:
            {
               int16_t result;
               mamaMsgField_getI16(field, &result);
               mamaMsg_addI16(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_U16:
            {
               uint16_t result;
               mamaMsgField_getU16(field, &result);
               mamaMsg_addU16(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_I32:
            {
               int32_t result;
               mamaMsgField_getI32(field, &result);
               mamaMsg_addI32(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_U32:
            {
               uint32_t result;
               mamaMsgField_getU32(field, &result);
               mamaMsg_addU32(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_I64:
            {
               int64_t result;
               mamaMsgField_getI64(field, &result);
               mamaMsg_addI64(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_U64:
            {
               uint64_t result;
               mamaMsgField_getU64(field, &result);
               mamaMsg_addU64(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_F32:
            {
               mama_f32_t result;
               mamaMsgField_getF32(field, &result);
               mamaMsg_addF32(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_F64:
            {
               mama_f64_t result;
               mamaMsgField_getF64(field, &result);
               mamaMsg_addF64(newMsg, fieldName, fid, result);
               break;
            }

         case MAMA_FIELD_TYPE_TIME:
            {
               mamaDateTime result = NULL;
               //char dateTimeString[56];
               mamaDateTime_create (&result);
               mamaMsgField_getDateTime(field, result);
               //                    mamaDateTime_getAsString (result,dateTimeString, 56);
               mamaMsg_addDateTime(newMsg, fieldName, fid, result);
               mamaDateTime_destroy(result);
               break;
            }

         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice result;
               //char priceString[56];
               mamaPrice_create(&result);
               mamaMsgField_getPrice(field, result);
               //                    mamaPrice_getAsString (result, priceString, 56);
               mamaMsg_addPrice(newMsg, fieldName, fid, result);
               mamaPrice_destroy(result);
               break;
            }

         case MAMA_FIELD_TYPE_STRING:
            {
               const char *result = NULL;
               mamaMsgField_getString(field, &result);
               mamaMsg_addString(newMsg, fieldName, fid, result);
               break;
            }

         default:
            break;
         }
         //displayField (field, msg, 0);
      }

      msgStatus = getNextMsgStatus();
      mamaMsg_addI32(newMsg, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, msgStatus);

      printf("!!! republishing msg status %s\n", mamaMsgStatus_stringForStatus(msgStatus));
      mamaPublisher_send(gPublisher, newMsg);

//      status = mamaPublisher_sendFromInbox(gPublisher, gInbox, newMsg);

   }

   mamaMsgIterator_destroy(iterator);

   //        mamaPublisher_sendReplyToInbox (gPublisher, msg, msg);
   //        mamaPublisher_send (gPublisher, msg);
}

static void publishMessage(mamaMsg request)
{
   static int msgNumber = 0;
   static int sentInitial = 0;
   static mamaMsg msg = NULL;
   mamaMsgStatus msgStatus;

   mamaPrice mamaAskPrice;
   mamaPrice mamaBidPrice;

   // test code for dictionary
   //   mamaFieldDescriptor field = NULL;
   //short fid;

   static double bidPrice = 100.0;
   static double askPrice = 101.0;

   static int bidSize = 1000;
   static int askSize = 1000;

   mama_status status;

   //status = mamaDictionary_getFieldDescriptorByName (gDictionary, &field, "ASK");
   //fid = mamaFieldDescriptor_getFid (field);

   if (msg == NULL)
   {
      status = mamaMsg_create(&msg);
   }
   else
   {
      status = mamaMsg_clear(msg);
   }

   if (status != MAMA_STATUS_OK)
   {
      printf("Error creating/clearing msg: %s\n",
         mamaStatus_stringForStatus(status));
      exit(status);
   }

   /* Add some fields. This is not required, but illustrates how to
   * send data.
   */
   msgStatus = getNextMsgStatus();
   if ((status = mamaMsg_addI32(msg, MamaFieldMsgStatus.mName, MamaFieldMsgStatus.mFid, msgStatus)) != MAMA_STATUS_OK)
   {
      printf("Error %d adding status %d to msg\n", status, msgStatus);
      exit(status);
   }

   if ((status = mamaMsg_addI32(msg, MamaFieldSeqNum.mName, MamaFieldSeqNum.mFid, msgNumber)) != MAMA_STATUS_OK)
   {
      printf("Error %d adding sequence number %d to msg\n", status, msgNumber);
      exit(status);
   }

   // now add some data fields

   mamaPrice_create(&mamaAskPrice);
   mamaPrice_create(&mamaBidPrice);

   mamaPrice_setValue(mamaAskPrice, askPrice);
   mamaPrice_setValue(mamaBidPrice, bidPrice);

   status = mamaMsg_addPrice(msg, "wAskPrice", 109, mamaAskPrice);
   status = mamaMsg_addPrice(msg, "wBidPrice", 237, mamaBidPrice);
   //status = mamaMsg_addU64(msg, "wAskSize", 110, askSize);
   //status = mamaMsg_addU64(msg, "wBidSize", 238, bidSize);

   if (request)
   {
      if (gQuietLevel < 1)
      {
         printf("\nPublishing message %d to inbox .", msgNumber);
      }
      status = mamaMsg_addU8(msg, MamaFieldMsgType.mName, MamaFieldMsgType.mFid, MAMA_MSG_TYPE_RECAP);
      status = mamaPublisher_sendReplyToInbox(gPublisher, request, msg);
   }
   else
   {
      if (!sentInitial)
      {
         status = mamaMsg_addU8 (msg, MamaFieldMsgType.mName, MamaFieldMsgType.mFid, MAMA_MSG_TYPE_INITIAL);
         sentInitial = 1;
      }
      else
      {
         status = mamaMsg_addU8 (msg, MamaFieldMsgType.mName, MamaFieldMsgType.mFid, MAMA_MSG_TYPE_UPDATE);
      }

      // status = mamaPublisher_send (gPublisher, msg);  // no inbox for feedback with this call

      if (gNumSymbols > 0)
      {
         // was a list that was read from a file
         int index;
         for (index = 0; index < gNumSymbols; index++)
         {
            if (gQuietLevel < 1)
            {
               printf("Publishing message %d to %s.\n", msgNumber, gSymbolList[index]);
               fflush(stdout);
            }
            status = mamaMsg_addString(msg, "PublisherTopic", 10002,  gSymbolList[index]);
            status = mamaPublisher_sendFromInbox(gPublisherList[index], gInbox, msg);
         }
      }
      else
      {
         if (gQuietLevel < 1)
         {
            printf("Publishing message %d to %s.", msgNumber, gOutBoundTopic);
            fflush(stdout);
         }
         status = mamaMsg_addString(msg, "PublisherTopic", 10002, gOutBoundTopic);
         status = mamaPublisher_sendFromInbox(gPublisher, gInbox, msg);
         if (gQuietLevel < 1)
         {
            const char* subject=NULL;
            mamaMsg_getSendSubject(msg, &subject);
            if (subject)
            {
               printf("\tsubject=%s", subject);
            }
         }
         printf("\tmsg=%s\n", mamaMsg_toString (msg));
      }
   }

   if (status != MAMA_STATUS_OK)
   {
      printf("Error sending msg: %s\n", mamaStatus_stringForStatus (status));
      exit (status);
   }
   msgNumber++;

   askPrice += 1.0;
   bidPrice += 1.0;
   askSize += 10;
   bidSize += 10;

   mamaPrice_destroy(mamaAskPrice);
   mamaPrice_destroy(mamaBidPrice);
}

static void createSubscription()
{
   mama_status status;
   mamaMsgCallbacks callbacks;

   if (strlen(gSubSymbol) == 0)
   {
      // no sub symbol - just return, the bridge will do offstream posts
      return;
   }

   //memset(&gInboundCb, 0, sizeof(gInboundCb));
   //gInboundCb.onCreate         = inboundCreateCb;
   //gInboundCb.onError          = inboundErrorCb;
   //gInboundCb.onMsg            = inboundMsgCb;
   //gInboundCb.onQuality        = NULL; /* not used by basic subscriptions */
   //gInboundCb.onGap            = NULL; /* not used by basic subscriptions */
   //gInboundCb.onRecapRequest   = NULL; /* not used by basic subscriptions */

   memset(&callbacks, 0, sizeof(callbacks));
   callbacks.onCreate  = subscriptionOnCreate;
   callbacks.onError   = subscriptionOnError;
   callbacks.onMsg     = subscriptionOnMsg;
   callbacks.onQuality = NULL;
   callbacks.onGap     = NULL;
   callbacks.onRecapRequest = NULL;

   // create a mama source
   /*The mamaSource used for all subscription creation*/
   if (MAMA_STATUS_OK!=(status=mamaSource_create (&gSubscriptionSource)))
   {
      fprintf (stderr,
         "Failed to create subscription mamaSource STATUS: %d %s\n",
         status, mamaStatus_stringForStatus (status));
      exit(1);
   }

   mamaSource_setId (gSubscriptionSource, "Subscription_Source");
   mamaSource_setTransport (gSubscriptionSource, gTransport);
   mamaSource_setSymbolNamespace (gSubscriptionSource, gServiceSub);

   status = mamaSubscription_allocate (&gSubscription);

   // should check for valid source and sub name here
   status = mamaSubscription_create(gSubscription,gMamaDefaultQueue, &callbacks, gSubscriptionSource, gSubSymbol, 0);

   if (status != MAMA_STATUS_OK)
   {
      printf("Error creating subscription: %s\n",
         mamaStatus_stringForStatus (status));
      exit (status);
   }
}

static void MAMACALLTYPE subscriptionOnCreate(mamaSubscription subscription, void *closure)
{
   if (gQuietLevel < 2)
   {
      printf("Created inbound subscription.\n");
   }
}

static void MAMACALLTYPE subscriptionOnError(mamaSubscription subscription, mama_status status,
   void *platformError, const char *subject, void *closure)
{
   printf("Error creating subscription: %s\n",
      mamaStatus_stringForStatus (status));
   exit (status);
}

static void MAMACALLTYPE subscriptionOnMsg(mamaSubscription subscription, mamaMsg msg,
   void *closure, void *itemClosure)
{

   if (gQuietLevel < 2)
   {
      printf("Received inbound msg. (%s) Sending response\n", mamaMsg_toString(msg));
   }

   //if (!mamaMsg_isFromInbox (msg))
   //{
   //    printf("Inbound msg not from inbox. No reply sent.\n");
   //    return;
   //}
   if (gRePub == 1)
   {
      republish(msg);
      return;
   }

   displayAllFields (msg, subscription, 0);
   //    publishMessage (msg);
}

static void createIntervalTimer()
{
   mama_status status;

   status = mamaTimer_create(&gTimer, gMamaDefaultQueue,
      timerCallback, gInterval, NULL);

   if (status != MAMA_STATUS_OK)
   {
      printf("Error creating timer: %s\n",
         mamaStatus_stringForStatus (status));
      exit (status);
   }
}

static void createPubDelayTimer()
{
   mama_status status;

   status = mamaTimer_create (&gPubDelayTimer,
      gMamaDefaultQueue,
      pubDelayTimerCallback,
      gPubDelay,
      NULL);

   if (status != MAMA_STATUS_OK)
   {
      printf("Error creating timer: %s\n",
         mamaStatus_stringForStatus (status));
      exit(status);
   }
}

static void MAMACALLTYPE timerCallback(mamaTimer timer, void *closure)
{
   if (gRePub == 0)
   {
      publishMessage(NULL);
   }
}

static void MAMACALLTYPE pubDelayTimerCallback(mamaTimer timer, void *closure)
{
   createPublisher();

   // stop this timer
   mamaTimer_destroy(timer);

   // and create a ticker for publishing values
   createIntervalTimer();
}

void parseCommandLine(int argc, const char **argv)
{
   int i = 0;
   for (i = 1; i < argc; )
   {
      if (strcmp ("-pub", argv[i]) == 0)
      {
         gOutBoundTopic = argv[i+1];
         i += 2;
      }
      else if (strcmp (argv[i], "-f") == 0)
      {
         gFilename = argv[i + 1];
         i += 2;
      }
      else if (strcmp ("-sub", argv[i]) == 0)
      {
         gSubSymbol = argv[i+1];
         i += 2;
      }
      else if (strcmp ("-Ss", argv[i]) == 0)
      {
         gServiceSub = argv[i+1];
         i += 2;
      }
      else if (strcmp ("-Sp", argv[i]) == 0)
      {
         gServicePub = argv[i+1];
         i += 2;
      }
      //else if (strcmp ("-l", argv[i]) == 0)
      //{
      //    gInBoundTopic = argv[i+1];
      //    i += 2;
      //}
      else if ((strcmp (argv[i], "-h") == 0)||
         (strcmp (argv[i], "--help") == 0)||
         (strcmp (argv[i], "-?") == 0))
      {
         usage(0);
      }
      else if (strcmp ("-i", argv[i]) == 0)
      {
         gInterval = strtod (argv[i+1], NULL);
         i += 2;
      }
      else if (strcmp ("-tport", argv[i]) == 0)
      {
         gTransportName = argv[i+1];
         i += 2;
      }
      else if (strcmp ("-q", argv[i]) == 0)
      {
         gQuietLevel++;
         i++;
      }
      else if (strcmp ("-R", argv[i]) == 0)
      {
         gRePub = 1;
         i++;
      }
      else if (strcmp ("-m", argv[i]) == 0)
      {
         gMiddleware = argv[i+1];
         i += 2;
      }
      else if (strcmp("-status", argv[i]) == 0)
      {
         gStatusLiveCount = atoi(argv[i+1]);
         gStatusStaleCount = atoi(argv[i+2]);
         i += 3;
      }
      else if (strcmp (argv[i], "-use_dict_file") == 0)
      {
          gDictFromFile = 1;
          gBuildDataDict =0;
          gDictFile = strdup (argv[i+1]);
          i+=2;
      }
      else if (strcmp (argv[i], "-v") == 0)
      {
         if (mama_getLogLevel() == MAMA_LOG_LEVEL_WARN)
         {
            mama_enableLogging(stderr, MAMA_LOG_LEVEL_NORMAL);
         }
         else if (mama_getLogLevel() == MAMA_LOG_LEVEL_NORMAL)
         {
            mama_enableLogging(stderr, MAMA_LOG_LEVEL_FINE);
         }
         else if (mama_getLogLevel() == MAMA_LOG_LEVEL_FINE)
         {
            mama_enableLogging(stderr, MAMA_LOG_LEVEL_FINER);
         }
         else
         {
            mama_enableLogging(stderr, MAMA_LOG_LEVEL_FINEST);
         }
         i++;
      }
      else if (strcmp ("-shutdown", argv[i]) == 0)
      {
          gShutdownTime = atoi (argv[i + 1]);
          i += 2;
      }


      else
      {
         printf("Unknown arg: %s\n",    argv[i]);
         i++;
      }
   }

   if (gQuietLevel < 2)
   {
      printf("Starting Publisher with:\n"
         "   topic:              %s\n"
         "   subscription symbol:      %s\n"
         "   interval            %f\n"
         "   transport:          %s\n",
         gOutBoundTopic, gSubSymbol, gInterval, gTransportName);
   }
}

void usage(int exitStatus)
{
   int i=0;
   while (gUsageString[i]!=NULL)
   {
      printf("%s\n", gUsageString[i++]);
   }
   exit (exitStatus);
}

void displayAllFields(mamaMsg msg, mamaSubscription subscription, int indentLevel)
{
   mamaMsgField field = NULL;
   mama_status status = MAMA_STATUS_OK;

   const char* source = NULL;
   const char* symbol = NULL;
   const char* issueSymbol = NULL;

   mamaSubscription_getSource (subscription, &source);
   mamaSubscription_getSymbol (subscription, &symbol);
   mamaMsg_getString (msg, NULL, 305, &issueSymbol);
   printf("%s.%s.%s Type: %s Status %s \n",
      issueSymbol == NULL ? "" : issueSymbol,
      source      == NULL ? "" : source,
      symbol      == NULL ? "" : symbol,
      mamaMsgType_stringForMsg(msg),
      mamaMsgStatus_stringForMsg(msg));

   /*
   Iterating over all the fields in a message is more efficient than
   accessing data directly on the message object itself. When accessing
   the message directly the message must first find the field before
   returning the data. For messages which do not contain the requested
   field this requires traversal of the whole message before returning.
   Using the iteration approach requires that each field within the
   message is only visited once.
   */
   mamaMsg_iterateFields(msg, displayCb, gDictionary, (void *) indentLevel);
}

#define printData(value, format)    \
   do                               \
   {                                \
      if (gQuietLevel==0)           \
      {                             \
         printf (format, value);    \
      }                             \
   } while (0)

void displayField (mamaMsgField field, const mamaMsg msg, int indentLevel)
{
   mamaFieldType   fieldType       = MAMA_FIELD_TYPE_UNKNOWN;
   const char*     fieldName       = NULL;
   const char*     fieldTypeName   = NULL;
   uint16_t        fid             = 0;
   const char*     indentOffset    = NULL;
   const char*     indentOffsetAll = "%-20s | %4d | %13s | ";

   /*For formatting of output only*/
   switch (indentLevel)
   {
   case 0:
      indentOffset = "%2s";
      break;
   case 1:
      indentOffset = "%4s";
      break;
   case 2:
      indentOffset = "%6s";
      break;
   case 3:
      indentOffset = "%8s";
      break;
   default:
      indentOffset = "%10s";
   }

   if (gQuietLevel < 1)
   {
      printData("", indentOffset);
      /*
      Attributes for a field can be obtained directly from the field
      rather than from the field descriptor describing that field.
      */
      mamaMsgField_getName(field, &fieldName);
      mamaMsgField_getFid(field, &fid);
      mamaMsgField_getTypeName(field, &fieldTypeName);
      printf (indentOffsetAll, fieldName ? fieldName : "", fid, fieldTypeName);
   }

   /*
   The most efficient way of extracting data while iterating fields is to
   obtain the field type and then call the associated strongly typed
   accessor.

   mamaMsgField_getAsString() will return a stringified version of the
   data but is considerably less efficient and is not recommended for
   production use.
   */
   mamaMsgField_getType(field, &fieldType);

   switch (fieldType)
   {
   case MAMA_FIELD_TYPE_BOOL:
      {
         mama_bool_t result;
         mamaMsgField_getBool(field, &result);
         printData(result, "%d\n");
         break;
      }

   case MAMA_FIELD_TYPE_CHAR:
      {
         char result;
         mamaMsgField_getChar (field, &result);
         printData(result, "%c\n");
         break;
      }


   case MAMA_FIELD_TYPE_I8:
      {
         int8_t result;
         mamaMsgField_getI8 (field, &result);
         printData(result, "%d\n");
         break;
      }

   case MAMA_FIELD_TYPE_U8:
      {
         uint8_t result;
         mamaMsgField_getU8(field, &result);
         printData(result, "%u\n");
         break;
      }

   case MAMA_FIELD_TYPE_I16:
      {
         int16_t result;
         mamaMsgField_getI16(field, &result);
         printData(result, "%d\n");
         break;
      }

   case MAMA_FIELD_TYPE_U16:
      {
         uint16_t result;
         mamaMsgField_getU16(field, &result);
         printData(result, "%u\n");
         break;
      }

   case MAMA_FIELD_TYPE_I32:
      {
         int32_t result;
         mamaMsgField_getI32(field, &result);
         printData(result, "%d\n");
         break;
      }

   case MAMA_FIELD_TYPE_U32:
      {
         uint32_t result;
         mamaMsgField_getU32(field, &result);
         printData(result, "%u\n");
         break;
      }

   case MAMA_FIELD_TYPE_I64:
      {
         int64_t result;
         mamaMsgField_getI64(field, &result);
         printData(result, "%lld\n");
         break;
      }

   case MAMA_FIELD_TYPE_U64:
      {
         uint64_t result;
         mamaMsgField_getU64(field, &result);
         printData(result, "%llu\n");
         break;
      }

   case MAMA_FIELD_TYPE_F32:
      {
         mama_f32_t result;
         mamaMsgField_getF32(field, &result);
         printData(result, "%f\n");
         break;
      }
   case MAMA_FIELD_TYPE_F64:
      {
         mama_f64_t result;
         mamaMsgField_getF64(field, &result);
         printData(result, "%f\n");
         break;
      }

   case MAMA_FIELD_TYPE_TIME:
      {
         mamaDateTime result = NULL;
         char         dateTimeString[56];
         mamaDateTime_create(&result);
         mamaMsgField_getDateTime(field, result);
         mamaDateTime_getAsString(result,dateTimeString, 56);
         printData(dateTimeString, "%s\n");
         mamaDateTime_destroy(result);
         break;
      }

   case MAMA_FIELD_TYPE_PRICE:
      {
         mamaPrice result;
         char priceString[56];
         mamaPrice_create(&result);
         mamaMsgField_getPrice(field, result);
         mamaPrice_getAsString(result, priceString, 56);
         printData(priceString, "%s\n");
         mamaPrice_destroy(result);
         break;
      }

   case MAMA_FIELD_TYPE_STRING:
      {
         const char* result = NULL;
         mamaMsgField_getString(field, &result);
         printData(result, "%s\n");
         break;
      }

   case MAMA_FIELD_TYPE_VECTOR_STRING:
      {
         const char **result = NULL;
         mama_size_t size = 0;
         mama_size_t i;
         mamaMsgField_getVectorString(field, &result, &size);
         printData("", "%s\n");
         for (i = 0; i < size; i++)
         {
            printData("", indentOffset);
            printData(result[i], "  %s\n");
         }
         break;
      }

   case MAMA_FIELD_TYPE_VECTOR_I32:
      {
         const mama_i32_t* result = NULL;
         mama_size_t size = 0;
         mama_size_t i =0;
         mamaMsgField_getVectorI32 (field, &result, &size);
         printData("","%s\n");
         for (i = 0; i < size; i++)
         {
            printData("", indentOffset);
            printData(result[i], " %d\n");
         }
         break;
      }

   case MAMA_FIELD_TYPE_VECTOR_F64:
      {
         const mama_f64_t *result = NULL;
         mama_size_t size = 0;
         mama_size_t i = 0;
         mamaMsgField_getVectorF64(field, &result, &size);
         printData("","%s\n");
         for (i = 0; i < size; i++)
         {
            printData("", indentOffset);
            printData(result[i], " %f\n");
         }
         break;
      }

   case MAMA_FIELD_TYPE_MSG:
      {
         mamaMsg result;
         mamaMsgField_getMsg(field, &result);
         printData("", "%s\n");
         displayAllFields(result, NULL, indentLevel + 1);
         break;
      }

   case MAMA_FIELD_TYPE_VECTOR_MSG:
      {
         /*
         Vectors of messages are only supported using WomabtMsg as
         the wire encoding data format. The example here
         recursively prints all data in all messages.
         */
         const mamaMsg*  result;
         mama_size_t     resultLen;
         mama_size_t     i;
         mamaMsgField_getVectorMsg(field, &result, &resultLen);
         printData("", "%s\n");
         printData("", indentOffset);
         printData("{", "%s\n");
         for (i = 0; i < resultLen; i++)
         {
            displayAllFields(result[i], NULL, indentLevel + 1);
         }
         printData("", indentOffset);
         printData("}", "%s\n");
         break;
      }
   default:
      {
         printData("Unknown", "%s\n");
      }
      break;
   }/*End switch*/
}

void MAMACALLTYPE displayCb(const mamaMsg msg, const mamaMsgField field, void *closure)
{
   displayField(field, msg, (int)closure);
}

void MAMACALLTYPE inboxOnMessage(mamaMsg msg, void *closure)
{
   printf("inbox message from publisher \n");
   mamaMsg_iterateFields(msg, displayCb, gDictionary, (void *) 0);
}

static void createInbox()
{
   mamaInbox_create(&gInbox, gTransport, gMamaDefaultQueue, inboxOnMessage, 0, 0);
}

static void readSymbolsFromFile()
{
   /* get subjects from file or interactively */
   FILE* fp = NULL;
   char charbuf[1024];

   if (gFilename)
   {
      if ((fp = fopen (gFilename, "r")) == (FILE *) NULL)
      {
         perror(gFilename);
         exit(1);
      }
   }
   else
   {
      fp = stdin;
   }
   if (isatty(fileno(fp)))
   {
      printf("Enter one symbol per line and terminate with a .\n");
      printf("Symbol> ");
   }
   while (fgets(charbuf, 1023, fp))
   {
      char *c = charbuf;

      /* Input terminate case */
      if (*c == '.')
         break;

      /* replace newlines with NULLs */
      while ((*c != '\0') && (*c != '\n'))
         c++;
      *c = '\0';

      gSymbolList[gNumSymbols++] = strdup (charbuf);
      if (isatty(fileno(fp)))
      {
         printf("Symbol> ");
      }
   }
}

static void buildDataDictionary(void)
{
   mamaDictionaryCallbackSet dictionaryCallback;
   mama_status result  =   MAMA_STATUS_OK;

   if (gDictFromFile)
       {
           mamaDictionary_create (&gDictionary);
           mamaDictionary_populateFromFile(
               gDictionary,
               gDictFile);
           return;
    }

   memset(&dictionaryCallback, 0, sizeof(dictionaryCallback));
   dictionaryCallback.onComplete = completeCb;
   dictionaryCallback.onTimeout  = timeoutCb;
   dictionaryCallback.onError    = errorCb;

   /*
   The dictionary is created asynchronously. The dictionary has been
   successfully retrieved once the onComplete callback has been
   invoked.
   */
   result = mama_createDictionary(&gDictionary, gMamaDefaultQueue, /* Use the default event queue */
      dictionaryCallback, gDictSource, 10.0, 3, NULL); /* Don't specify a closure */

   if (result != MAMA_STATUS_OK)
   {
      fprintf (stderr, "Exception creating dictionary: MamaStatus: %s\n",
         mamaStatus_stringForStatus(result));
      exit (1);
   }

   /*
   Start dispatching on the default event queue. Dispatching on the
   queue is unblocked once one of the dictionary callbacks is invoked.
   */
   mama_start(gMamaBridge);

   /*
   True only if onComplete resulted in the unblocking of the queue
   dispatching
   */
   if (!gDictionaryComplete)
   {
      fprintf(stderr, "Could not create dictionary.\n" );
      exit(1);
   }
}

void MAMACALLTYPE timeoutCb(mamaDictionary dict, void *closure)
{
   printf("Timed out waiting for dictionary\n" );
   mama_stop(gMamaBridge);
}

void MAMACALLTYPE errorCb(mamaDictionary dict, const char *errMsg, void *closure)
{
   fprintf(stderr, "Error getting dictionary: %s\n", errMsg );
   mama_stop(gMamaBridge);
}

void MAMACALLTYPE completeCb(mamaDictionary dict, void *closure)
{
   gDictionaryComplete = 1;

   /* Stop processing events until all subscriptions have been created. */
   mama_stop(gMamaBridge);
}

static void mamashutdown (void)
{
    /* destroy the publisher  */
    if(NULL != gPublisher)
    {
        mamaPublisher_destroy(gPublisher);
    }

    /* Destroy the source. */
    if (NULL != gSubscriptionSource)
    {
        mamaSource_destroy(gSubscriptionSource);
    }

    /* Destroy the dictionary. */
    if (gDictionary != NULL)
    {
        fprintf(stderr, "mamashutdown: destroying dictionary\n");
        mamaDictionary_destroy(gDictionary);
    }

    /* Destroy the dictionary source. */
    if (NULL != gDictSource)
    {
        mamaSource_destroy(gDictSource);
    }

    if (gTransport != NULL)
    {
        fprintf(stderr, "mamashutdown: destroying transport\n");
        mamaTransport_destroy (gTransport);
    }

    mama_close ();
}

echo  *** copying OpenMama files
rem %1=$(Configuration) %2="$(OutDir)" %3=[UPA build version root directory dpends on the compiler version]

xcopy %TICK42_OPENMAMA%\%1\mamalistenc.exe %2 /FY
xcopy %TICK42_OPENMAMA%\%1\mamachurnc.exe %2 /FY
xcopy %TICK42_OPENMAMA%\%1\libwombatcommonmdd.dll %2 /FY
xcopy %TICK42_OPENMAMA%\%1\libmamacmdd.dll %2 /FY
xcopy %TICK42_OPENMAMA%\%1\libmamacppmdd.dll %2 /FY

if NOT [%3] == [] xcopy %TICK42_UPA%\Libs\%3\Debug_MDd\Shared\librssl.dll %2 /FY

rem for atomic book ticker
xcopy %TICK42_OPENMAMA%\%1\libmamacppmdd.dll %2 /FY
xcopy %TICK42_OPENMAMA%\%1\libmamaentnoopmdd.dll %2 /FY
xcopy %TICK42_OPENMAMA%\%1\libmamdabookmdd.dll %2 /FY
xcopy %TICK42_OPENMAMA%\%1\libmamdamdd.dll %2 /FY
xcopy %TICK42_OPENMAMA%\%1\openmamdaatomicbookticker.exe %2 /FY

xcopy %TICK42_OPENMAMA%\%1\openmamdabookticker.exe %2 /FY

echo *** copying local files ***
xcopy .\mama.properties %2 /FY
xcopy .\mamalistenc.cmd %2 /FY
xcopy .\rundemo.cmd %2 /FY
xcopy .\mama_dict.txt %2 /FY

echo *** copying fieldsmap.csv file ***
xcopy .\fieldmap.csv %2 /FY

echo *** copying RMDS dictionaries ***
xcopy %TICK42_UPA%\etc\enumtype.def %2 /FY 
xcopy %TICK42_UPA%\etc\RDMFieldDictionary %2 /FY 
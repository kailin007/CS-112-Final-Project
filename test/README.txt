
Step1: configurate "--prefix=YourOwnDirectory" in the "cunit_install.sh" 
       and run it to install CUnit using command "bash cunit_install.sh"

Step2: change the "LD_LIBRARY_PATH" & "PKG_CONFIG_PATH" to your own directory in the "cunit_env.sh" 
       and run it to setup environment using command "bash cunit_env.sh"

Step3: change the "INC" & LIB" to your own directory in the Makefile
       and run it to complie test files using command "make"

Step4: open the proxy and run command "./out" to start automated testing 

Step5: the output results will be included in the "results" directory, which are the two files with ".xml" format.
       Then use IE browser to open the two files to see the results (other browers may not work)
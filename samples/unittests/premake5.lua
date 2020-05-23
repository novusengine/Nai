UNITTEST_NAME = "Nai Unittests"

TESTOUTPUT_BASE_PATH = '%{OUTPUT_BASE_PATH}%{cfg.buildcfg}//testresults/'
UNITTEST_FILE_PATH = '%{file.path:gsub("../samples/unittests/", "")}'
UNITTEST_RESULT_PATH = '%{UNITTEST_FILE_PATH}.result'

TESTOUTPUT_PATH = '%{TESTOUTPUT_BASE_PATH .. UNITTEST_RESULT_PATH}'

project(UNITTEST_NAME)
    kind "Utility"
    location "../../build"
    filename (UNITTEST_NAME)
    uuid "3F3B938C-ABC6-0051-B4D7-834520E25C53"
    targetdir "../../bin"
    dependson { PROJECT_NAME }

    files { "**.nai", "**.nai.result" }
    filter("files:**.nai")
        --buildmessage 'Running test %{file.path:gsub("../samples/unittests/", "")}: '
        --buildmessage 'Running: "../bin/%{cfg.buildcfg}/Nai_%{cfg.buildcfg}.exe"'
        --buildmessage "%{TESTOUTPUT_PATH}"
        buildcommands { '"../bin/%{cfg.buildcfg}/Nai_%{cfg.buildcfg}.exe" unittest testoutput="%{TESTOUTPUT_PATH}" ../samples/unittests/%{UNITTEST_FILE_PATH}'}
        buildoutputs { '../../bin/%{cfg.buildcfg}/testresults/%{file.path:gsub("../samples/unittests/", "")}.result'}
    
    filter("files:**.nai.test")
        flags("ExcludeFromBuild")

// cli_cfiles.cc for Fluxbox Window Manager
// Copyright (c) 2014 - Mathias Gumz <akira at fluxbox.org>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "cli.hh"
#include "defaults.hh"

#include "Debug.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/I18n.hh"
#include "FbTk/Resource.hh"
#include "FbTk/StringUtil.hh"

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/types.h>
#include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif

#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif


using std::string;
using std::endl;
using std::cerr;


#ifdef _WIN32
/**
 Wrapper function for Windows builds - mkdir takes only one param.
*/
static int mkdir(const char *dirname, int /*permissions*/) {
    return mkdir(dirname);
}
#endif


/**
 setup the configutation files in
 home directory
*/
void FluxboxCli::setupConfigFiles(const std::string& dirname, const std::string& rc) {

    _FB_USES_NLS;

    const bool has_dir = FbTk::FileUtil::isDirectory(dirname.c_str());


    struct CFInfo {
        bool create_file;
        const char* default_name;
        const std::string filename;
    } cfiles[] = {
        { !has_dir, DEFAULT_INITFILE, rc },
        { !has_dir, DEFAULTKEYSFILE, dirname + "/keys" },
        { !has_dir, DEFAULTMENU, dirname + "/menu" },
        { !has_dir, DEFAULT_APPSFILE, dirname + "/apps" },
        { !has_dir, DEFAULT_OVERLAY, dirname + "/overlay" },
        { !has_dir, DEFAULT_WINDOWMENU, dirname + "/windowmenu" }
    };
    const size_t nr_of_cfiles = sizeof(cfiles)/sizeof(CFInfo);


    if (has_dir) { // check if anything with these names exists, if not create new
        for (size_t i = 0; i < nr_of_cfiles; ++i) {
            cfiles[i].create_file = access(cfiles[i].filename.c_str(), F_OK);
        }
    } else {

        fbdbg << "Creating dir: " << dirname << endl;
        if (mkdir(dirname.c_str(), 0700)) {
            fprintf(stderr, _FB_CONSOLETEXT(Fluxbox, ErrorCreatingDirectory,
                                    "Can't create %s directory",
                                    "Can't create a directory, one %s for directory name").c_str(),
                    dirname.c_str());
            cerr << endl;
            return;
        }
    }

    bool sync_fs = false;

    // copy default files if needed
    for (size_t i = 0; i < nr_of_cfiles; ++i) {
        if (cfiles[i].create_file) {
            FbTk::FileUtil::copyFile(FbTk::StringUtil::expandFilename(cfiles[i].default_name).c_str(), cfiles[i].filename.c_str());
            sync_fs = true;
        }
    }
#ifdef HAVE_SYNC
    if (sync_fs) {
       sync();
    }
#endif
}



// configs might be out of date, so run fluxbox-update_configs
// if necassary.
void FluxboxCli::updateConfigFilesIfNeeded(const std::string& rc_file) {

    FbTk::ResourceManager r_mgr(rc_file.c_str(), false);
    FbTk::Resource<int> c_version(r_mgr, 0, "session.configVersion", "Session.ConfigVersion");

    if (!r_mgr.load(rc_file.c_str())) {
        _FB_USES_NLS;
        cerr << _FB_CONSOLETEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "")
            << ": " 
            << rc_file << endl;
        return;
    }

    if (*c_version < CONFIG_VERSION) {

        fbdbg << "updating config files from version " 
            << *c_version
            << " to "
            << CONFIG_VERSION
            << endl;

        string commandargs = realProgramName("fluxbox-update_configs");
        commandargs += " -rc " + rc_file;

        if (system(commandargs.c_str())) {
            fbdbg << "running '" 
                << commandargs
                << "' failed." << endl;
        }
#ifdef HAVE_SYNC
        sync();
#endif // HAVE_SYNC
    }
}



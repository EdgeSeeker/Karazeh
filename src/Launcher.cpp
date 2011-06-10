/*
 *  Copyright (c) 2011 Ahmad Amireh <ahmad@amireh.net>
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#include "Launcher.h"
#include "PixyLogLayout.h"
#include "Renderers/Vanilla/VanillaRenderer.h"
#ifdef KARAZEH_RENDERER_OGRE
#include "Renderers/Ogre/OgreRenderer.h"
#endif
#ifdef KARAZEH_RENDERER_QT
#include "Renderers/Qt/QtRenderer.h"
#endif

#if PIXY_PLATFORM == PIXY_PLATFORM_WIN32
#include <windows.h>
#include <tchar.h>
#endif

namespace Pixy
{
	Launcher* Launcher::__instance = 0;

  void handle_interrupt(int param)
  {
    printf("Signal %d received, shutdown is forced; attempting to cleanup. Please see the log.\n", param);
    Launcher::getSingleton().requestShutdown();
  }

	Launcher::Launcher() :
	mRenderer(0),
  mVWorker(0),
  mPWorker(0) {
	  signal(SIGINT, handle_interrupt);
	  signal(SIGTERM, handle_interrupt);
	  //signal(SIGKILL, handle_interrupt);
	}

	Launcher::~Launcher() {

    if (mRenderer)
      delete mRenderer;

    delete Downloader::getSingletonPtr();
    delete Patcher::getSingletonPtr();

    if (mVWorker)
      delete mVWorker;
    if (mPWorker)
      delete mPWorker;
    mVWorker = mPWorker = 0;

		mLog->infoStream() << "++++++ " << PIXY_APP_NAME << " cleaned up successfully ++++++";
		if (mLog)
		  delete mLog;

		log4cpp::Category::shutdown();

		mRenderer = NULL;
	}

	Launcher* Launcher::getSingletonPtr() {
		if( !__instance ) {
		    __instance = new Launcher();
		}

		return __instance;
	}

	Launcher& Launcher::getSingleton() {
		return *getSingletonPtr();
	}

	void Launcher::go(int argc, char** argv) {

    this->resolvePaths();

		this->initLogger();

		Patcher::getSingletonPtr();
		Downloader::getSingletonPtr();

    this->initRenderer(argc, argv);

    this->updateApplication();

		// main loop
    return mRenderer->go();
	}

  void Launcher::resolvePaths() {

    using boost::filesystem::path;

#if PIXY_PLATFORM == PIXY_PLATFORM_LINUX
    // use binreloc and boost::filesystem to build up our paths

    //BrInitError* brerr = 0;
    int brres = br_init(0);

    if (brres == 0) {
      std::cerr << "binreloc could not be initialised\n";
    }
    //if (brerr != 0)
    //  delete brerr;

    char *p = br_find_exe_dir(".");
    mBinPath = std::string(p);
    free(p);
    mBinPath = path(mBinPath).string();
    mRootPath = path(mBinPath).remove_leaf().string();
    mTempPath = path(mRootPath + "/" + std::string(PROJECT_TEMP_DIR)).string();
    mLogPath = path(mRootPath + "/" + std::string(PROJECT_LOG_DIR)).string();
#elif PIXY_PLATFORM == PIXY_PLATFORM_APPLE
    // use NSBundlePath() to build up our paths
#else
    // use GetModuleFileName() and boost::filesystem to build up our paths on Windows
    TCHAR szPath[MAX_PATH];

    if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
    {
        printf("Cannot install service (%d)\n", GetLastError());
        return;
    }

    std::cout << szPath << "\n";
    mBinPath = std::string(szPath);
    mBinPath = path(mBinPath).remove_filename().make_preferred().string();
    mRootPath = path(mBinPath).remove_leaf().make_preferred().string();
    mTempPath = path(mRootPath + "/" + std::string(PROJECT_TEMP_DIR)).make_preferred().string();
    mLogPath = path(mRootPath + "/" + std::string(PROJECT_LOG_DIR)).make_preferred().string();    
#endif

//#ifdef DEBUG
    std::cout << "Binary path: " <<  mBinPath << "\n";
    std::cout << "Root path: " <<  mRootPath << "\n";
    std::cout << "Temp path: " <<  mTempPath << "\n";
    std::cout << "Log path: " <<  mLogPath << "\n";
//#endif

  };

  void Launcher::initRenderer(int argc, char** argv) {

		if (argc > 1) {
#ifdef KARAZEH_RENDERER_OGRE
		  if (strcmp(argv[1], "Ogre") == 0)
		    mRenderer = new OgreRenderer();
#endif
#ifdef KARAZEH_RENDERER_QT
		  if (strcmp(argv[1], "Qt") == 0)
		    mRenderer = new QtRenderer();
#endif
      if (!mRenderer) {
        mLog->errorStream() << "unknown renderer specified! going vanilla";
      }
		}

    if (!mRenderer) {
      mRenderer = new VanillaRenderer();
    }

    bool res = mRenderer->setup(argc, argv);

    if (!res) {
      mLog->errorStream() << "could not initialise renderer!";
      return;
    }

  }
  std::string& Launcher::getRootPath() {
    return mRootPath;
  };
  std::string& Launcher::getTempPath() {
    return mTempPath;
  };
  std::string& Launcher::getBinPath() {
    return mBinPath;
  };
	void Launcher::requestShutdown() {
    if (mRenderer)
      mRenderer->cleanup();

	}

	void Launcher::initLogger() {
    using boost::filesystem::path;
    using boost::filesystem::exists;
    using boost::filesystem::create_directory;

    // TODO: fix other OSes paths
		std::string lLogPath = mLogPath;
#if PIXY_PLATFORM == PIXY_PLATFORM_WINDOWS
		lLogPath = path(mLogPath + "/" + "Karazeh.log").string();
#elif PIXY_PLATFORM == PIXY_PLATFORM_APPLE
		lLogPath = macBundlePath() + "/Contents/Logs/Launcher.log";
#else
		lLogPath = path(mLogPath + "/" + "Karazeh.log").make_preferred().string();
#endif

    if (!exists(path(mLogPath)))
      create_directory(path(mLogPath));
    std::cout << "Karazeh: initting log4cpp logger @ " << lLogPath << "!\n";

		log4cpp::Appender* lApp = new
		log4cpp::FileAppender("FileAppender", lLogPath, false);

    log4cpp::Layout* lLayout = new PixyLogLayout();
		/* used for header logging */
		PixyLogLayout* lHeaderLayout = new PixyLogLayout();
		lHeaderLayout->setVanilla(true);

		lApp->setLayout(lHeaderLayout);

		std::string lCatName = PIXY_LOG_CATEGORY;
		log4cpp::Category* lCat = &log4cpp::Category::getInstance(lCatName);

    lCat->setAdditivity(false);
		lCat->setAppender(lApp);
		lCat->setPriority(log4cpp::Priority::DEBUG);

		lCat->infoStream()
		<< "\n+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+";
		lCat->infoStream()
		<< "\n+                                 " << PIXY_APP_NAME << "                                    +";
		lCat->infoStream()
		<< "\n+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n";

		lApp->setLayout(lLayout);

		lApp = 0;
		lCat = 0;
		lLayout = 0;
		lHeaderLayout = 0;

		mLog = new log4cpp::FixedContextCategory(PIXY_LOG_CATEGORY, "Launcher");
	}

	void Launcher::launchExternalApp() {
    using boost::filesystem::path;
    path appPath = path(mBinPath + "/" + PIXY_EXTERNAL_APP_PATH);

#if PIXY_PLATFORM == PIXY_PLATFORM_WIN32
    //ShellExecute(appPath.string());
#else
    // to pass more arguments to the app, you need to change this line to reflect it
    execl(appPath.c_str(), PIXY_EXTERNAL_APP_NAME, PIXY_EXTERNAL_APP_ARG, NULL);
#endif
	}

  Renderer* Launcher::getRenderer() {
    return mRenderer;
  }

  void Launcher::updateApplication() {
    if (mVWorker)
      mPWorker = new Thread<Patcher>(Patcher::getSingleton());
    else
      mVWorker = new Thread<Patcher>(Patcher::getSingleton());
  }
} // end of namespace Pixy

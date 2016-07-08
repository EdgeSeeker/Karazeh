/**
 * karazeh -- the library for patching software
 *
 * Copyright (C) 2011-2016 by Ahmad Amireh <ahmad@amireh.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "karazeh/path_resolver.hpp"

namespace kzh {
  namespace fs = boost::filesystem;

  static path_t locate_bin_directory(const logger*);
  static path_t locate_root_directory(const path_t&);

  path_resolver::path_resolver()
  : logger("path_resolver")
  {
  }

  path_resolver::~path_resolver() {
  }

  void path_resolver::resolve(path_t root) {
    if (root.empty()) {
      root_path_ = locate_root_directory(locate_bin_directory(this));
    }
    else {
      root_path_ = root;
    }

    // locate the binary and build its path
    #if KZH_PLATFORM == KZH_PLATFORM_LINUX
      // Linux:
      debug() << "Platform: Linux";

      cache_path_ = path_t(root_path_ / ".kzh" / "cache").make_preferred();
    #elif KZH_PLATFORM == KZH_PLATFORM_APPLE
      // OS X:
      debug() << "Platform: OS X";

      cache_path_ = path_t(root_path_ / "/Resources/.kzh/cache").make_preferred();
    #else
      // Windows:
      debug() << "Platform: Windows";

      cache_path_ = path_t(root_path_ / ".kzh/cache").make_preferred();
    #endif

    debug() << "Root path: " <<  root_path_;
    debug() << "Cache path: " <<  cache_path_;
  }

  path_t const& path_resolver::get_root_path() const {
    return root_path_;
  }

  path_t const& path_resolver::get_cache_path() const {
    return cache_path_;
  }

  path_t locate_bin_directory(const logger* log) {
    // locate the binary and build its path
    #if KZH_PLATFORM == KZH_PLATFORM_LINUX
      // use binreloc and fs to build up our paths
      int brres = br_init(0);
      if (brres == 0) {
        log->error() << "binreloc could not be initialised";
        throw internal_error("Unable to resolve paths! binreloc could not be initialized");
      }

      char *tmp_bin_path = br_find_exe_dir(".");
      path_t bin_path = path_t(tmp_bin_path).make_preferred();
      free(tmp_bin_path);

      return bin_path;
    #elif KZH_PLATFORM == KZH_PLATFORM_APPLE
      log->debug() << "Platform: OS X";

      // use NSBundlePath() to build up our paths
      return path_t(Utility::macBundlePath() + "/Contents/MacOS").make_preferred();
    #else // Windows
      // use GetModuleFileName() and fs to build up our paths on Windows
      TCHAR szPath[MAX_PATH];

      if (!GetModuleFileName(NULL, szPath, MAX_PATH)) {
        log->error() << "Unable to resolve path: " << GetLastError();;
        throw internal_error("Unable to resolve paths! GetModuleFileName() failed. See the log for the error.");
      }

      return path_t(string_t(szPath)).remove_filename().make_preferred();
    #endif
  }

  // On Windows and Linux, root is KZH_DISTANCE_FROM_ROOT directories up from
  // the binary's
  path_t locate_root_directory(const path_t& bin_path) {
    #if KZH_PLATFORM == KZH_PLATFORM_APPLE
      return path_t(bin_path).remove_leaf().make_preferred();
    #else
      path_t root_path(bin_path);

      for (int i = 0; i < KZH_DISTANCE_FROM_ROOT; ++i) {
        root_path = root_path.remove_leaf();
      }

      return root_path.make_preferred();
    #endif
  }
}
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

#include "karazeh/file_manager.hpp"

namespace kzh {
  namespace fs = boost::filesystem;

  file_manager::file_manager() : logger("file_manager")
  {
  }

  file_manager::~file_manager() {
  }

  bool file_manager::load_file(std::ifstream &fs, string_t& out_buf) const
  {
    if (!fs.is_open() || !fs.good()) return false;

    while (fs.good()) {
      out_buf.push_back(fs.get());
    }

    out_buf.erase(out_buf.size()-1,1);

    return true;
  }

  bool file_manager::load_file(string_t const& path, string_t& out_buf) const
  {
    bool rc;
    std::ifstream fs(path.c_str());

    try {
      rc = load_file(fs, out_buf);
    }
    catch(...) {
      fs.close();
      throw;
    }

    fs.close();

    return rc;
  }

  bool file_manager::load_file(path_t const& path, string_t& out_buf) const
  {
    return load_file(path.string(), out_buf);
  }

  bool file_manager::is_readable(string_t const& resource) const
  {
    path_t path(resource);

    try {
      if (fs::exists(path)) {
        if (fs::is_directory(path)) {
          for (fs::directory_iterator it(path); it != fs::directory_iterator(); ++it) {
            break;
          }

          return true;
        }
        else {
          std::ifstream fs(resource.c_str());
          bool readable = fs.is_open() && fs.good();
          fs.close();
          return fs::is_regular_file(path) && readable;
        }
      }
    }
    catch (fs::filesystem_error &e) {
      return false;
    }

    return false;
  }

  bool file_manager::is_readable(path_t const& resource) const
  {
    return is_readable(path_t(resource).make_preferred().string());
  }

  bool file_manager::is_writable(string_t const& resource) const
  {
    try {
      path_t path(resource);

      if (fs::exists(path)) {

        if (is_directory(path)) {
          return is_writable(path / "__karazeh_internal_directory_check__");
        }

        // it already exists, make sure we don't overwrite it
        std::ofstream fs(resource.c_str(), std::ios_base::app);
        bool writable = fs.is_open() && fs.good() && !fs.fail();
        fs.close();

        return fs::is_regular_file(path) && writable;
      } else {

        // try creating a file and write to it
        std::ofstream fs(resource.c_str(), std::ios_base::app);
        bool writable = fs.is_open() && fs.good() && !fs.fail();
        fs << "This was generated automatically by Karazeh and should have been deleted.";
        fs.close();

        if (fs::exists(path)) {
          // delete the file
          fs::remove(path);
        }

        return writable;
      }
    }
    catch (fs::filesystem_error &e) {
      // something bad happened, it is most likely unwritable
      return false;
    }

    return false;
  }

  bool file_manager::is_empty(path_t const& path) const
  {
    return fs::is_empty(path);
  }

  bool file_manager::is_writable(path_t const& resource) const
  {
    return is_writable(path_t(resource).make_preferred().string());
  }

  bool file_manager::is_directory(path_t const& path) const
  {
    return is_readable(path) && fs::is_directory(path);
  }

  bool file_manager::create_directory(path_t const& path) const
  {
    try {
      fs::create_directories(path);
    }
    catch (fs::filesystem_error &e) {
      error()
        << "Unable to create directory chain @ " << path
        << ". Cause: " << e.what();

      return false;
    }
    catch (std::exception &e) {
      error() << "Unknown error while creating directory chain @ " << path;
      error() << "Possible cause: " << e.what();

      return false;
    }

    return true;
  }

  bool file_manager::ensure_directory(path_t const& path) const
  {
    if (is_directory(path)) {
      return true;
    }
    else {
      return create_directory(path);
    }
  }

  bool file_manager::make_executable(path_t const& p) const
  {
    using namespace fs;

    try {
      permissions(p, owner_all | group_exe | others_exe);
    } catch (fs::filesystem_error &e) {
      error() << "Unable to modify permissions of file: " << p;
      return false;
    }

    return true;
  }

  uint64_t file_manager::stat_filesize(std::ifstream& in) const
  {
    in.seekg(0,std::ifstream::end);
    uint64_t size = in.tellg();
    in.seekg(0);

    return size;
  }

  uint64_t file_manager::stat_filesize(path_t const& p) const
  {
    std::ifstream fp(p.string().c_str(), std::ios_base::binary);
    if (!fp.is_open() || !fp.good())
      return 0;

    uint64_t size = stat_filesize(fp);

    fp.close();

    return size;
  }

  bool file_manager::remove_file(const path_t& path) const {
    if (!is_writable(path)) {
      return false;
    }

    try {
      fs::remove(path);
    }
    catch (fs::filesystem_error &e) {
      return false;
    }

    return true;
  }

  bool file_manager::remove_directory(const path_t& path) const {
    if (!is_readable(path)) {
      return false;
    }

    try {
      fs::remove_all(path);
    }
    catch (fs::filesystem_error &e) {
      return false;
    }

    return true;
  }

  bool file_manager::exists(const path_t& path) const {
    try {
      return fs::exists(path);
    }
    catch (fs::filesystem_error &e) {
      return false;
    }
  }

  bool file_manager::move(path_t const& src, path_t const& dst) const {
    if (exists(src) && !exists(dst)) {
      try {
        fs::rename(src, dst);

        return true;
      }
      catch (fs::filesystem_error) {
        return false;
      }
    }
    else {
      return false;
    }
  }
}
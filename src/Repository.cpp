/*
 *  COpyright (c) 2011 Ahmad Amireh <ahmad@amireh.net>
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  cOpy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, cOpy, modify, merge, publish, distribute, sublicense,
 *  and/or sell cOpies of the Software, and to permit persons to whom the 
 *  Software is furnished to do so, subject to the following conditions:
 *  
 *  The above cOpyright notice and this permission notice shall be included in 
 *  all cOpies or substantial portions of the Software.
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
 
#include "Repository.h"

namespace Pixy {
	
	Repository::Repository(Version inVersion) : mVersion(inVersion) {
	  mVersion = inVersion;
	  mLog = 
	  new log4cpp::FixedContextCategory(
	    PIXY_LOG_CATEGORY,
	     "Repository: " + mVersion.Value
	  );
		mLog->infoStream() << "firing up";
		
  }
	
	Repository::~Repository() {
		
		mLog->infoStream() << "shutting down";
		
		PatchEntry* lEntry = 0;
		while (!mEntries.empty()) {
		  lEntry = mEntries.back();
		  mEntries.pop_back();
		  delete lEntry;
		}
		lEntry = 0;
		
		if (mLog)
		  delete mLog;
	}

	
  void 
  Repository::registerEntry(std::string Local, 
                         std::string Remote, 
                         PATCHOP Op)
  {
    mLog->infoStream() << "Registering patch entry of type " <<
      ( (Op == CREATE) ? "CREATE" : (Op == MODIFY) ? "MODIFY" : "DELETE" )
      << " with src: " << Local << " and dest: " << Remote;
     
    PatchEntry *lEntry = new PatchEntry();
    
    lEntry->Op = Op; 
    lEntry->Local = Local;
    lEntry->Remote = Remote;
    lEntry->Repo = this;
    mEntries.push_back(lEntry);
    lEntry = 0;
  }
  
  std::vector<PatchEntry*>
  Repository::getEntries(PATCHOP inOp) {
  
    std::vector<PatchEntry*> entries;
    std::vector<PatchEntry*>::const_iterator _itr;
    for (_itr = mEntries.begin(); _itr != mEntries.end(); ++_itr) {
      if ((*_itr)->Op == inOp)
        entries.push_back((*_itr));
    }
  
    return entries;
  }
};
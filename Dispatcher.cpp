#include "asardll.h"

#include <iostream>
#include <iterator>
#include <vector> 
#include <algorithm>
#include <fstream>

using namespace std;

vector<char> ROMdata;

#define Exit(cmdLine, e)  { if(!cmdLine) { cout << "\nPress enter to exit... "; getc(stdin); } else { cout << endl; } return e; }

int main(int argc, char *argv[])
{

  string cleanROM;
  string dirtyROM;
  string patchASM;

  // Are we using command line?
  bool e = true;
    
  cout << "Dispatcher by wiiqwertyuiop\n\n";
  
  // Get user input
  if (argc == 4) {
    
    patchASM = argv[1];
    dirtyROM = argv[2];
    cleanROM = argv[3];
    
  } else if (argc == 1) {
    
    e = false;
    
    cout << "Enter patch to remove: ";
    getline(cin, patchASM);
        
    cout << "Enter ROM to remove patch from: ";
    getline(cin, dirtyROM);
    
    cout << "Enter unmodified ROM: ";
    getline(cin, cleanROM);
    
    cout << endl;
    
  } else {
  
    cout << "Usage: " << argv[0] << " <patch> <patched ROM> <unmodified ROM>\n";
    return -1;
  
  }
  
  // Check patch file exists and can be used
	ifstream patchASM_file(patchASM, ifstream::binary);
	if (!patchASM_file.good()) {
		cout << "Error!: Can't open patch file: [" << patchASM << "]";
		Exit(e, -1)
	}
	patchASM_file.close();

  // Open dirty ROM
  ifstream patchedROM_file(dirtyROM, ifstream::ate | ifstream::binary);
  
  int header = 0;
  if (patchedROM_file.is_open() && patchedROM_file.good()) {
  
    ifstream::pos_type size = patchedROM_file.tellg();
    ifstream::pos_type pos = 0;
    
    if (size % 0x8000 == 512) {
      size -= 512;
      pos += 512;
      header = 512;
    }
    
    ROMdata.resize(size);
    
    patchedROM_file.seekg(pos);
    patchedROM_file.read(&ROMdata[0], size);

		patchedROM_file.close();
    
  } else {
    cout << "Error!: Can't open patched ROM file: [" << dirtyROM << "]";
		Exit(e, -1)
  }
  
  // Get asar
  if (!asar_init()) {
		cout << "Error!: Couldn't initialize Asar. Ensure [asar.dll] exists and is in the same folder as the tool.\n";
		Exit(e, -1)
	}
  
  // Patch file to the already patched ROM so we can see what data gets changed
  int ROMdata_size = ROMdata.size();
  if (!asar_patch(patchASM.c_str(), &ROMdata[0], ROMdata_size, &ROMdata_size)) {
		cout << "Asar gave an error(s) with the patch file:\n";

		int error_count;
		const errordata* errors = asar_geterrors(&error_count);

		for (int i = 0; i < error_count; i++)
		{
			cout << errors[i].fullerrdata << endl;
		}
    
    Exit(e, -1)
	}

  // Now open clean ROM
  ifstream cleanROM_file(cleanROM, ifstream::ate | ifstream::binary);
  
  int header2 = 0;
  ifstream::pos_type size = 0;
  
  if (cleanROM_file.is_open() && cleanROM_file.good()) {
  
    size = cleanROM_file.tellg();
    
    if (size % 0x8000 == 512) {
      header2 = 512;
    }
    
  } else {
    cout << "Error!: Can't open clean ROM file: [" << cleanROM << "]";
		Exit(e, -1)
  }
  
  // Now get the list of changes
  int numwrittenblocks=0;
	const writtenblockdata * writtenblocks = asar_getwrittenblocks(&numwrittenblocks);

	for (int i = 0; i < numwrittenblocks; i++) {
	  	
	  int PClocation = writtenblocks[i].pcoffset;
	  
	  if(PClocation > size) {

	    for(int a = 0; a < writtenblocks[i].numbytes; a++) {
        ROMdata[PClocation+a] = '\0';
      }
      
	  } else {
	    
      for(int a = 0; a < writtenblocks[i].numbytes; a++) {
        cleanROM_file.seekg(header2+PClocation+a);
        unsigned int out = cleanROM_file.get();   
        ROMdata[PClocation+a] = out;
      }
	  }
	  
	}
	
	// Close clean ROM
	cleanROM_file.close();
	
	// Output unpatched ROM
	ofstream unpatchedROM_file;
	unpatchedROM_file.open(dirtyROM, ofstream::binary);
	ostream_iterator<unsigned char> output (unpatchedROM_file);
	
	// give the outputted ROM a header if the original one had one
	if(header != 0) {
	  unpatchedROM_file.width(header+1);
    unpatchedROM_file.fill(0);
	}

  copy ( ROMdata.begin(), ROMdata.end(), output );
  
	// Done
  asar_close();
  
  cout << "Done!";
  Exit(e, 0)
}

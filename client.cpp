/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Matthew Huang
	UIN: 934004247
	Date: 9/23/25
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	// file variables
	string filename = ""; 
	// int o = 0;
	// int l = 0;
	// server variable
	int m = 5000;
	
	while ((opt = getopt(argc, argv, "p:t:e:f:m:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			// case 'o':
			// 	o = atoi (optarg);
			// 	break;
			// case 'l':
			// 	l = atoi (optarg);
			// 	break;
			// server variable
			case 'm':
				m = atoi (optarg);
				break;
		}
	}

	// give args for server
	// ** CREATE SERVER AS CHILD ** 
	char mchar[20];
	sprintf(mchar, "%d", m);
	const char *argv_server[] = {"./server", "-m", mchar, NULL};
	int pid = fork();
	if (pid == 0){
		execvp(argv_server[0], (char**) argv_server);
	} else{

		FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
		
		// PATIENT data request 
		if (p != -1){
			char buf[MAX_MESSAGE]; // 256
			double reply;
			// ONE data point request
			if (t != -1.0){
				// BOTH ecg numbers (executes the question and answer twice)
				if (e == -1){
					e = 1; // first ecg number
					datamsg x(p, t, e);
					memcpy(buf, &x, sizeof(datamsg));
					chan.cwrite(buf, sizeof(datamsg)); // question
					chan.cread(&reply, sizeof(double)); //answer
					e = 2; // second ecg number
					cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
				}
				datamsg x(p, t, e);
				memcpy(buf, &x, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg)); // question
				chan.cread(&reply, sizeof(double)); //answer
				cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
			
			// first 1000 data points
			} else {
				FILE *fptr;
				string filepath = "received/x1.csv";
				fptr = fopen(filepath.c_str(), "w");

				// send requests to server
				for (int i = 0; i < 1000; i++){
					e = 1; // first ecg number
					t = i*0.004;
					// write time to file
					fprintf(fptr, "%.9g,", t);

					datamsg x(p, t, e);
					memcpy(buf, &x, sizeof(datamsg));
					chan.cwrite(buf, sizeof(datamsg)); // question
					chan.cread(&reply, sizeof(double)); //answer
					// write to file
					fprintf(fptr, "%.9g,", reply);

					e = 2; // second ecg number
					datamsg x2(p, t, e);
					memcpy(buf, &x2, sizeof(datamsg));
					chan.cwrite(buf, sizeof(datamsg)); // question
					chan.cread(&reply, sizeof(double)); //answer
					// write to file
					fprintf(fptr, "%.9g\n", reply);
				}

				fclose(fptr);
			}
		// FILE data request
		} else if (filename != ""){
			string fname = filename;

			// 1. Get file length 
			filemsg fm1(0, 0);
			int len = sizeof(filemsg) + (fname.size() + 1);
			char* buf2 = new char[len];
			memcpy(buf2, &fm1, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), fname.c_str());
			chan.cwrite(buf2, len);  // I want the file length;
			delete[] buf2;

			__int64_t filelength;
			chan.cread(&filelength, sizeof(__int64_t)); 
				
			FILE *fptr;
			string filepath = "received/" + fname;
			fptr = fopen(filepath.c_str(), "w");

			// 2. Get file contents
			for (__int64_t i = 0; i < filelength; i += (__int64_t) m){
				// send request to server
				int chunklen = min((__int64_t) m, filelength-i);

				filemsg fm2(i, chunklen);
				len = sizeof(filemsg) + (fname.size() + 1);
				buf2 = new char[len];
				memcpy(buf2, &fm2, sizeof(filemsg));
				strcpy(buf2 + sizeof(filemsg), fname.c_str());
				chan.cwrite(buf2, len);
				delete[] buf2;
				// get reply from server
				// write to file in packets of m bytes
				char* reply;
				reply = new char[chunklen];
				chan.cread(reply, chunklen); 
				fwrite(reply, 1, chunklen, fptr);
				delete[] reply;
			}

			fclose(fptr);
		}
			
		
				
		// closing the channel    
		MESSAGE_TYPE msg = QUIT_MSG;
		chan.cwrite(&msg, sizeof(MESSAGE_TYPE));
	}
}

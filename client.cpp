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
	int p = 1;
	double t = -1.0;
	int e = -1;
	// file variables
	string filename = "1.csv"; //TODO replace
	int o = 0;
	int l = 0;
	// server variable
	const char *m = "5000";
	
	while ((opt = getopt(argc, argv, "p:t:e:f:o:l:m:")) != -1) {
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
			case 'o':
				o = atoi (optarg);
				break;
			case 'l':
				l = atoi (optarg);
				break;
			// server variable
			case 'm':
				m = optarg;
				break;
		}
	}

	// give args for server
	// ** CREATE SERVER AS CHILD ** 
	const char *argv_server[] = {"./server", "-m", m, NULL};
	int pid = fork();
	if (pid == 0){
		execvp(argv_server[0], (char**) argv_server);
	} else{

		FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
		
		// ONE data point request
		if (t != -1.0){
			char buf[MAX_MESSAGE]; // 256
			double reply;
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
		
		// file data request
		} else {
			
			filemsg fm(o, l);
			string fname = filename;

			int len = sizeof(filemsg) + (fname.size() + 1);
			char* buf2 = new char[len];
			memcpy(buf2, &fm, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), fname.c_str());
			chan.cwrite(buf2, len);  // I want the file length;
			delete[] buf2;
			// Get Reply:
			// special case: want file length
			if (o == 0 && l == 0){
				__int64_t reply;
				chan.cread(&reply, sizeof(__int64_t)); 
				cout << "File length:" << reply << endl;
			// normal case: write to file
			} else {
				FILE *fptr;
				fptr = fopen("received/data.txt", "w");

				// if (fptr == NULL) {
				// 	perror("Failed to open file");
				// 	delete[] reply;  // Clean up memory if fopen fails
				// 	return 1;  // Or handle error accordingly
				// }

				char* reply = new char[fm.length];
				for (int i = 0; i < l; i++){
					chan.cread(&reply, fm.length); 
					fprintf(fptr, "%s", reply);
				}
				fclose(fptr);
				delete[] reply;

			}

		}
				




		
		// closing the channel    
		MESSAGE_TYPE m = QUIT_MSG;
		chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	}
}

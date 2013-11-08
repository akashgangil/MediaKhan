
/**
 **    File: khan.cpp
 **    Description: Main functionality for the khan filesystem.
 **/

#include "khan.h"

#define SELECTOR_C '@'
#define SELECTOR_S "@"

#define log stderr




//mkdir stats prints stats to stats file and console:
string stats_file="./stats.txt";
string rename_times_file_name="./rename_times.txt";
string start_times_file_name = "./start_times.txt";
vector<string> servers;
vector<string> server_ids;
string this_server;
string this_server_id;
//PyObject* cloud_interface;
ofstream rename_times;
ofstream start_times;
/*
void cloud_upload(string path) {
	FILE* stream=popen(("id3convert -1 -2 '"+path+"'").c_str(),"r");
	pclose(stream);
	//cout << " Uploading song " << path << endl;
	PyObject* arglist = PyTuple_New(1);
	PyTuple_SetItem(arglist, 0, PyString_FromString(path.c_str()));
	PyObject* myFunction =PyObject_GetAttrString(cloud_interface,(char*)"upload_song");
	PyObject* myResult = PyObject_CallObject(myFunction, arglist);
	if(myResult==NULL) {
		PyErr_PrintEx(0);
	}
}

void cloud_download(string song, string path) {
	//cout << " Downloading song " << song << " to " << path << endl;
	PyObject* arglist = PyTuple_New(2);
	PyTuple_SetItem(arglist, 0, PyString_FromString(song.c_str()));
	PyTuple_SetItem(arglist, 1, PyString_FromString(path.c_str()));
	PyObject* myFunction = PyObject_GetAttrString(cloud_interface,(char*)"get_song");
	PyObject* myResult = PyObject_CallObject(myFunction, arglist);
	if(myResult==NULL) {
		PyErr_PrintEx(0);
	}
	FILE* stream=popen(("id3convert -1 '"+path+"'").c_str(),"r");
	pclose(stream);
}
*/

void process_transducers(string server) {

	cout << "Process Transducers\n";

	if(server == "cloud") {
		return;
	}
	string line;
	ifstream transducers_file((server+"/transducers.txt").c_str());
	getline(transducers_file, line);
	while(transducers_file.good()){
		cout << "=============== got type =   " << line <<endl;
		//add line to vold as file type
		database_setval("allfiles","types",line);
		database_setval(line,"attrs","name");
		database_setval(line,"attrs","tags");
		database_setval(line,"attrs","location");
		database_setval("namegen","command","basename");
		database_setval(line,"attrs","ext");
		string ext=line;
		getline(transducers_file,line);
		const char *firstchar=line.c_str();
		while(firstchar[0]=='-') {
			//add line to vold under filetype as vold
			stringstream ss(line.c_str());
			string attr;
			getline(ss,attr,'-');
			getline(ss,attr,':');
			string command;
			getline(ss,command,':');
			cout << "============ checking attr = "<<attr<<endl;
			cout << "============ checking command = "<<command<<endl;
			attr=trim(attr);
			database_setval(ext,"attrs",attr);
			database_setval(attr+"gen","command",command);
			getline(transducers_file,line);
			firstchar=line.c_str();
		}
	}
}

/*processes files: issues the mp3info command for the file
  and fills in the values of the attributes*/
void process_file(string server, string fileid) {
	printf("Inside the Process File \n");
	string file = database_getval(fileid, "name");
	string ext = database_getval(fileid, "ext");
	file = server + "/" + file;
	string attrs=database_getval(ext,"attrs");
	string token="";
	stringstream ss2(attrs.c_str());
	while(getline(ss2,token,':')){
		if(strcmp(token.c_str(),"null")!=0){
			if(token == "name") {
				continue;
			}
			cout << "========= looking at attr =   " << token <<endl;
			string cmd=database_getval(token+"gen","command");
			if(cmd=="null") {
				cout << "command is null, skipping" << endl;
				continue;
			}
			string msg2=(cmd+" \""+file+"\"").c_str();
			cout << "========= issuing command =   " << msg2 <<endl;
			FILE* stream=popen(msg2.c_str(),"r");
			if(fgets(msg,200,stream)!=0){
				cout << "========= attr value =   " << msg <<endl;
				database_setval(fileid,token,msg);
			}
			pclose(stream);
		}
	}
}

void map_path(string path, string fileid) {
	//cout << "in map_path" << endl;
	//cout << "path: " << path << " fileid: " << fileid << endl;
	string token = "";
	string attr = "";
	stringstream ss2(path.c_str());
	while(getline(ss2, token, '/')) {
		//cout << "got token " << token << endl;
		if(strcmp(token.c_str(),"null")!=0) {
			if(attr.length()>0) {
				//cout << "mapping " << attr << " to " << token << endl;
				database_setval(fileid, attr, token);
				if(attr=="location") {
					int pos=find(server_ids.begin(),server_ids.end(),token)-server_ids.begin();
					if( pos < server_ids.size() ) {
						database_setval(fileid, "server", servers.at(pos));
					}
				}
				attr = "";
			} else {
				attr = token;
			}
		}
	}
	//cout << "finished map_path" << endl << endl;
}

void unmap_path(string path, string fileid) {
	//cout << "in unmap_path" << endl;
	//cout << "path: " << path << " fileid: " << fileid << endl;
	string token = "";
	string attr = "";
	stringstream ss2(path.c_str());
	while(getline(ss2, token, '/')) {
		//cout << "got token " << token << endl;
		if(strcmp(token.c_str(),"null")!=0) {
			if(attr.length()>0) {
				//cout << "removing map " << attr << " to " << token << endl;
				database_remove_val(fileid, attr, token);
				if(attr=="location") {
					int pos=find(server_ids.begin(),server_ids.end(),token)-server_ids.begin();
					if( pos < server_ids.size() ) {
						database_remove_val(fileid, "server", servers.at(pos));
					}
				}
				attr = "";
			} else {
				attr = token;
			}
		}
	}
	//cout << "finished unmap_path" << endl << endl;
}

/*unmount the fuse file system*/
void unmounting(string mnt_dir) {
	//log_msg("in umounting");
#ifdef APPLE
	string command = "umount " + mnt_dir + "\n";
#else
	string command = "/net/hu21/agangil3/fuse-2.9.3/util/fusermount -u " + mnt_dir + "\n";
#endif
	if (system(command.c_str()) < 0) {  
		sprintf(msg,"Could not unmount mounted directory!\n");
		//log_msg(msg);
		return;
	}
	//log_msg("fusermount successful\n");
}

int initializing_khan(char * mnt_dir) {
	//log_msg("In initialize\n");
	unmounting(mnt_dir);
	//Opening root directory and creating if not present
	//cout<<"khan_root[0] is "<<servers.at(0)<<endl;
	if(NULL == opendir(servers.at(0).c_str()))  {
		sprintf(msg,"Error msg on opening directory : %s\n",strerror(errno));
		//log_msg(msg);
		//log_msg("Root directory might not exist..Creating\n");
		string command = "mkdir " + servers.at(0);
		if (system(command.c_str()) < 0) {
			log_msg("Unable to create storage directory...Aborting\n");
			exit(1);
		}
	} else {
		fprintf(stderr, "directory opened successfully\n");
	}

	init_database();

	//check if we've loaded metadata before
	string output=database_getval("setup","value");
	if(output.compare("true")==0){
		log_msg("Database was previously initialized.");
		tot_time+=(stop.tv_sec-start.tv_sec)+(stop.tv_nsec-start.tv_nsec)/BILLION;
		return 0; //setup has happened before
	}

	//if we have not setup, do so now
	//log_msg("it hasnt happened, setvalue then setup");
	database_setval("setup","value","true");

	cout << "I MA HERE!!!\n";

	//load metadata associatons
	for(int i=0; i<servers.size(); i++){
		cout << "servers: " << servers.at(i) << "\n";
		process_transducers(servers.at(i));
	}

	//load metadata for each file on each server
	string types=database_getval("allfiles","types");
	cout << "================= types to look for ="<<types<<endl;
	cout << "Server Size" << servers.size() << endl;
	for(int i=0; i<servers.size(); i++) {
/*		if(servers.at(i) == "cloud") {
			cout << " Cloud \n";
			PyObject* myFunction = PyObject_GetAttrString(cloud_interface,(char*)"get_all_titles");
			PyObject* myResult = PyObject_CallObject(myFunction, NULL);
			if(myResult==NULL) {
				PyErr_PrintEx(0);
				continue;
			}
			int n = PyList_Size(myResult);
			//cout << "SIZE = " << n << endl << flush;
			for(int j = 0; j<n; j++) {
				PyObject* title = PyList_GetItem(myResult, j);
				char* temp = PyString_AsString(title);
				if(temp==NULL) {
					PyErr_PrintEx(0);
					continue;
				}
				string filename = temp;
				//cout << "Checking " << filename << " ... " << endl << flush;
				if(database_getval("name",filename)=="null") {
					string fileid = database_setval("null","name",filename);
					string ext = strrchr(filename.c_str(),'.')+1;
					database_setval(fileid,"ext",ext);
					database_setval(fileid,"server",servers.at(i));
					database_setval(fileid,"location",server_ids.at(i));
					string attrs=database_getval(ext,"attrs");
					string token="";
					stringstream ss2(attrs.c_str());
					PyObject* myFunction = PyObject_GetAttrString(cloud_interface,(char*)"get_metadata");
					while(getline(ss2,token,':')){
						if(strcmp(token.c_str(),"null")!=0){
							//cout << "========= looking at attr =   " << token << endl << flush;
							PyObject* arglist = PyTuple_New(2);
							PyTuple_SetItem(arglist, 0, PyString_FromString(filename.c_str()));
							PyTuple_SetItem(arglist, 1, PyString_FromString(token.c_str()));
							PyObject* myResult = PyObject_CallObject(myFunction, arglist);
							//cout << myResult << endl << flush;
							if(myResult==NULL) {
								PyErr_PrintEx(0);
								continue;
							}
							char* msg = PyString_AsString(myResult);
							if(!msg) {
								PyErr_PrintEx(0);
								continue;
							}
							string val = msg;
							Py_DECREF(arglist);
							Py_DECREF(myResult);
							//cout << "========= got val =   " << val << endl << flush;
							if(val!="na") {
								database_setval(fileid,token,val);
							}
						}
					}
				}
			}
		} else {
*/
			cout << "Not Cloud \n";

//			string command = "find -type d | awk -F'/' '{print NF-1}' | sort -n | tail -1";
			string command = "find /net/hp100/ihpcae -type d | awk -F'/' '{print NF-1}' | sort -n | tail -1";

			glob_t files;
			string pattern="**";

			FILE* pipe = popen(command.c_str(), "r");
			if(!pipe) return -1; 

			char result[100];
			if(fgets(result,128, pipe) != NULL)
				cout<<"****RESULT******"<<result<<"\n";    

			int count = atoi(result);
			while(count--)
			{  
				cout << "Globbing with pattern " << pattern + ".mp3\n" << endl; 
				glob((pattern +".mp3").c_str(), 0, NULL, &files);
				cout << "Glob buffer" << files.gl_pathc << "\n";
				for(int j=0; j<files.gl_pathc; j++) {//for each file
					string file = files.gl_pathv[j];
					cout << "************File PATH : ***************" << file << "\n";
					string ext = strrchr(file.c_str(),'.')+1;
					string filename=strrchr(file.c_str(),'/')+1;
					if(database_getval("name", filename) == "null" || 1) {
						string fileid = database_setval("null","name",filename);
						database_setval(fileid,"ext",ext);
						database_setval(fileid,"server",servers.at(i));
						database_setval(fileid,"location",server_ids.at(i));
						for(int k=0; k<server_ids.size(); k++) {
							database_setval(fileid, server_ids.at(k), "0");
						}
						process_file(servers.at(i), fileid);
					} else {
						string fileid = database_getval("name",filename);
						database_setval(fileid,"server",servers.at(i));
						database_setval(fileid,"location",server_ids.at(i));
					}
				}
				pattern += "/*";
			}   


			//glob_t files;
			//glob((servers.at(i)+"/*.*").c_str(),0,NULL,&files);
			//cout << "******************Listing down the files***************" << "\n";
	//	}
		//log_msg("At the end of initialize\n");
		return 0;
	}
}

int khan_opendir(const char *c_path, struct fuse_file_info *fi) {
	return 0;
}

bool find(string str, vector<string> arr) {
	for(int i=0; i<arr.size(); i++) {
		if(str == arr[i]) return true;
	}
	return false;
}

string str_intersect(string str1, string str2) {
	vector<string> vec_1 = split(str1, ":");
	vector<string> vec_2 = split(str2, ":");
	vector<string> ret;
	for(int i=0; i<vec_1.size(); i++) {
		for(int j=0; j<vec_2.size(); j++) {
			if((vec_1[i]==vec_2[j]) && (!find(vec_1[i], ret))) {
				ret.push_back(vec_1[i]);
			}
		}
	}
	return join(ret,":");
}

string last_string = "";
vector<string> last_vector;
bool content_has(string vals, string val) {
	vector<string> checks;
	if(last_string == vals) {
		checks = last_vector;
	} else {
		checks = split(vals,":");
		last_string = vals;
		last_vector = checks;
	}
	for(int i=0; i<checks.size(); i++) {
		if(checks[i]==val) {
			return true;
		}
	}
	return false;
}

void dir_pop_stbuf(struct stat* stbuf, string contents) {
	time_t current_time;
	time(&current_time);
	stbuf->st_mode=S_IFDIR | 0755;
	stbuf->st_nlink=count_string(contents)+2;
	stbuf->st_size=4096;
	stbuf->st_mtime=current_time;
}

void file_pop_stbuf(struct stat* stbuf, string filename) {
	time_t current_time;
	time(&current_time);
	stbuf->st_mode=S_IFREG | 0666;
	stbuf->st_nlink=1;
	stbuf->st_size=get_file_size(filename);
	stbuf->st_mtime=current_time;
}

string resolve_selectors(string path) {
	//cout << "starting split" << endl << flush;
	vector<string> pieces = split(path, "/");
	//cout << "starting process" << endl << flush;
	for(int i=0; i<pieces.size(); i++) {
		//cout << "looking at " << pieces[i] << endl << flush;
		if(pieces[i].at(0)==SELECTOR_C) {
			//cout << "is a selector" << endl << flush;
			vector<string> selectores = split(pieces[i], SELECTOR_S);
			pieces[i]="";
			//cout << selectores.size() << " selectors to be exact" << endl << flush;
			for(int j=0; j<selectores.size(); j++) {
				//cout << "checking " << selectores[j] << endl << flush;
				bool matched = false;
				string content = database_getvals("attrs");
				//cout << "content " << content << endl << flush;
				vector<string> attr_vec = split(content, ":");
				//cout << "vs " << attr_vec.size() << " attrs" << endl << flush;
				//for all attrs
				for(int k=0; k<attr_vec.size(); k++) {
					//cout << "on " << attr_vec[k] << endl << flush;
					string vals = database_getvals(attr_vec[k]);
					//cout << "with " << vals << endl << flush;
					//see if piece is in vals
					if(content_has(vals, selectores[j])) {
						//if so piece now equals attr/val
						if(pieces[i].length()>0) {
							pieces[i]+="/";
						}
						matched = true;
						pieces[i]+=attr_vec[k]+"/"+selectores[j];
						break;
					}
				}
				if(!matched) {
					pieces[i]+="tags/"+selectores[j];
				}
			}
		}
	}
	string ret = join(pieces, "/");
	//cout << "selector path " << path << " resolved to " << ret << endl;
	return ret;
}

int populate_getattr_buffer(struct stat* stbuf, stringstream &path) {
	string attr, val, file, more;
	string current = "none";
	void* aint=getline(path, attr, '/');
	void* vint=getline(path, val, '/');
	void* fint=getline(path, file, '/');
	void* mint=getline(path, more, '/');
	bool loop = true;
	while(loop) {
		//cout << "top of loop" << endl << flush;
		loop = false;
		if(aint) {
			string query = database_getval("attrs", attr);
			//cout << content << "  " << attr << " " << query << endl;
			if(query!="null") {
				string content = database_getvals(attr);
				if(vint) {
					if(content_has(content, val) || (attr=="tags")) {
						string dir_content = database_getval(attr, val);
						if(current!="none") {
							dir_content = str_intersect(current, dir_content);
						}
						string attrs_content = database_getvals("attrs");
						if(fint) {
							string fileid = database_getval("name",file);
							if(content_has(dir_content, fileid)) {
								if(!mint) {
									// /attr/val/file path
									file_pop_stbuf(stbuf, file);
									return 0;
								}
							} else if(content_has(attrs_content, file)) {
								//repeat with aint = fint, vint = mint, etc
								aint = fint;
								attr = file;
								vint = mint;
								val = more;
								fint=getline(path, file, '/');
								mint=getline(path, more, '/');
								current = dir_content;
								loop = true;
							}
						} else {
							// /attr/val dir
							dir_pop_stbuf(stbuf, dir_content+attrs_content);
							return 0;
						}  
					} 
				} else { 
					// /attr dir
					dir_pop_stbuf(stbuf, content);
					return 0;
				}
			}
		} else {
			string types=database_getvals("attrs");
			dir_pop_stbuf(stbuf, types);
			return 0;
		}
	}
	return -2;
}

static int khan_getattr(const char *c_path, struct stat *stbuf) {

	//cout << "started get attr" << endl << flush;
	string pre_processed = c_path+1;
	if(pre_processed == ".DS_Store") {
		file_pop_stbuf(stbuf, pre_processed);
		return 0;
	}
	//cout << "starting to resolve selectors" << endl << flush;
	string after = resolve_selectors(pre_processed);
	stringstream path(after);
	//cout << "working to pop buffer" << endl << flush;
	//file_pop_stbuf(stbuf, "test");
	//int ret = 0;
	int ret = populate_getattr_buffer(stbuf, path);
	//cout << "ended get attr" << endl << flush;
	return ret;
}

void dir_pop_buf(void* buf, fuse_fill_dir_t filler, string content, bool convert) {
	vector<string> contents = split(content, ":");
	for(int i=0; i<contents.size(); i++) {
		if(convert) {
			string filename = database_getval(contents[i].c_str(), "name");
			filler(buf, filename.c_str(), NULL, 0);
		} else {
			filler(buf, contents[i].c_str(), NULL, 0);
		}
	}
}


void populate_readdir_buffer(void* buf, fuse_fill_dir_t filler, stringstream &path) {
	string attr, val, file, more;
	string current_content = "none";
	string current_attrs = "none";
	void* aint=getline(path, attr, '/');
	void* vint=getline(path, val, '/');
	void* fint=getline(path, file, '/');
	void* mint=getline(path, more, '/');
	bool loop = true;
	while(loop) {
		loop = false;
		string content = database_getvals("attrs");
		if(aint) {
			if(content_has(content, attr)) {
				current_attrs += ":";
				current_attrs += attr; 
				content = database_getvals(attr);
				if(vint) {
					if(content_has(content, val) || (attr=="tags")) {
						string dir_content = database_getval(attr, val);
						if(current_content!="none") {
							dir_content = intersect(current_content, dir_content);
						}
						string attrs_content = database_getvals("attrs");
						if(fint) {
							if(content_has(attrs_content, file)) {
								//repeat with aint = fint, vint = mint, etc
								aint = fint;
								attr = file;
								vint = mint;
								val = more;
								fint = getline(path, file, '/');
								mint = getline(path, more, '/');
								current_content = dir_content;
								loop = true;
							}
						} else {
							// /attr/val dir
							fprintf(stderr, "%s, %s\n\n\n\n\n\n", attrs_content.c_str(), current_attrs.c_str());
							attrs_content = subtract(attrs_content, current_attrs);
							dir_pop_buf(buf, filler, dir_content, true);
							dir_pop_buf(buf, filler, attrs_content, false);
						}  
					} 
				} else { 
					// /attr dir
					dir_pop_buf(buf, filler, content, false);
				}
			}
		} else {
			dir_pop_buf(buf, filler, content, false);
		}
	}
}

static int xmp_readdir(const char *c_path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi) {
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	string pre_processed = c_path+1;
	string after = resolve_selectors(pre_processed);
	stringstream path(after);
	populate_readdir_buffer(buf, filler, path);
	return 0;
}

int khan_open(const char *path, struct fuse_file_info *fi) {
	int retstat = 0;
	int fd;
	path = basename(strdup(path));
	//cout << "in khan_open with file " << path << endl << flush;
	// get file id
	string fileid = database_getval("name", path);
	// get server 
	string server = database_getval(fileid, "server");
	//cout << fileid << " " << server << endl << flush;
	if(server == "cloud") {
		//cout << "looking at cloud" << endl << flush; 
		string long_path = "/tmp/";
		long_path += path;
		//cout << "downloading to "<< long_path << endl << flush;
		//cloud_download(path, long_path);
	}
	return 0;
}


int xmp_access(const char *path, int mask)
{
	char *path_copy=strdup(path);
	if(strcmp(path,"/")==0) {
		//log_msg("at root");
		return 0;
	}
	//  if(strcmp(path,"/")==0) {
	log_msg("at root");
	return 0;
	//  }

	string dirs=database_getval("alldirs","paths");
	string temptok="";
	stringstream dd(dirs);
	while(getline(dd,temptok,':')){
		if(strcmp(temptok.c_str(),path)==0){
			return 0;
		}
	}

	int c=0;
	for(int i=0; path[i]!='\0'; i++){
		if(path[i]=='/') c++;
	}

	//decompose path
	stringstream ss0(path+1);
	string type, attr, val, file, more;
	void* tint=getline(ss0, type, '/');
	void* fint=getline(ss0, file, '/');
	void* mint=getline(ss0, more, '/');
	int reta=0;

	//check for filetype
	if(tint){
		string types = database_getval("allfiles","types");
		stringstream ss(types.c_str());
		string token;
		while(getline(ss,token,':')){
			if(strcmp(type.c_str(),token.c_str())==0){
				reta=1;
			}
		}
		int found=0;

		do{
			//get attr and val
			found=0;
			void *aint=fint;
			string attr=file;
			void *vint=mint;
			string val=more;
			fint=getline(ss0, file, '/');
			mint=getline(ss0, more, '/');

			//check for attr
			if(reta && aint) {
				//cout << attr << endl;
				string attrs= database_getval(type,"attrs");
				stringstream ss3(attrs.c_str());
				reta=0;
				while(getline(ss3,token,':')){
					if(strcmp(attr.c_str(), token.c_str())==0){
						reta=1;
					}
				}

				//check for val
				if(reta && vint) {
					cout << val << endl;
					if(strcmp(attr.c_str(),("all_"+type+"s").c_str())==0) {
						clock_gettime(CLOCK_REALTIME,&stop);
						time_spent = (stop.tv_sec-start.tv_sec)+(stop.tv_nsec-start.tv_nsec)/BILLION; tot_time += time_spent;;
						access_avg_time=(access_avg_time*(access_calls-1)+time_spent)/access_calls;
						return 0;
					}
					string vals=database_getvals(attr);
					stringstream ss4(vals.c_str());
					reta=0;
					while(getline(ss4,token,':')){
						cout << val << token << endl;
						if(strcmp(val.c_str(), token.c_str())==0){
							reta=1;
						}
					}

					//check for file
					if(reta && fint) {
						cout << file << endl;
						string files=database_getval(attr, val);
						stringstream ss4(files.c_str());
						if(!mint) {
							reta=0;
							while(getline(ss4,token,':')){
								token=database_getval(token,"name");
								if(strcmp(file.c_str(), token.c_str())==0){
									reta=1;
								}
							}
							stringstream ss5(attrs.c_str());
							while(getline(ss5,token,':')){
								if(strcmp(file.c_str(),token.c_str())==0){
									reta=1;
								}
							}
						} else {
							found=1;
						}
					}
				}
			}
		}while(found);
	}

	if(reta && !getline(ss0, val, '/')) {
		return 0;
	}
	path=append_path(path);
	int ret = access(path, mask);
	return ret;
}


static int xmp_mknod(const char *path, mode_t mode, dev_t rdev) {
	//log_msg("in xmp_mknod");

	path=append_path2(basename(strdup(path)));
	sprintf(msg,"khan_mknod, path=%s\n",path);
	//log_msg(msg);
	int res;
	if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1) {
		fprintf(stderr, "\nmknod error \n");
		return -errno;
	}
	return 0;
}


static int xmp_mkdir(const char *path, mode_t mode) {
	struct timespec mkdir_start, mkdir_stop;
	string strpath=path;
	if(strpath.find("localize")!=string::npos) {
		if(strpath.find("usage")!=string::npos) {
			usage_localize();
		} else {
			//cout << "LOCALIZING" << endl;
			//cout << strpath << endl;
			//check location
			string filename = "winter.mp3";
			string fileid = database_getval("name", filename);
			string location = get_location(fileid);
			string server = database_getval(fileid, "server");
			//cout << "======== LOCATION: " << location << endl << endl;
			//if not current
			if(location.compare(server)!=0) {
				//  move to new location
				//cout << " MUST MOVE "<<server<<" TO "<<location<<endl;
				database_setval(fileid,"server",location);
				string from = server + "/" + filename;
				string to = location + "/" + filename;
				string command = "mv " + from + " " + to;
				FILE* stream=popen(command.c_str(),"r");
				pclose(stream);
			}
		}
		//cout << "LOCALIZATION TIME:" << localize_time << endl <<endl;
		return -1;
	}
	if(strpath.find("stats")!=string::npos){
		//print stats and reset
		ofstream stfile;
		stfile.open(stats_file.c_str(), ofstream::out);
		stfile << "TOT TIME    :" << tot_time << endl;
		stfile << "Vold Calls   :" << vold_calls << endl;
		stfile << "     Avg Time:" << vold_avg_time << endl;
		stfile << "Readdir Calls:" << readdir_calls << endl;
		stfile << "     Avg Time:" << readdir_avg_time << endl;
		stfile << "Access Calls :" << access_calls << endl;
		stfile << "     Avg Time:" << access_avg_time << endl;
		stfile << "Read Calls   :" << read_calls << endl;
		stfile << "     Avg Time:" << read_avg_time << endl;
		stfile << "Getattr Calls:" << getattr_calls << endl;
		stfile << "     Avg Time:" << getattr_avg_time << endl;
		stfile << "Write Calls  :" << write_calls << endl;
		stfile << "     Avg Time:" << write_avg_time << endl;
		stfile << "Create Calls :" << create_calls << endl;
		stfile << "     Avg Time:" << create_avg_time << endl;
		stfile << "Rename Calls :" << rename_calls << endl;
		stfile << "     Avg Time:" << rename_avg_time << endl;
		stfile.close();
		//cout << "TOT TIME    :" << tot_time << endl;
		//cout << "Vold Calls   :" << vold_calls << endl;
		//cout << "     Avg Time:" << vold_avg_time << endl;
		//cout << "Readdir Calls:" << readdir_calls << endl;
		//cout << "     Avg Time:" << readdir_avg_time << endl;
		//cout << "Access Calls :" << access_calls << endl;
		//cout << "     Avg Time:" << access_avg_time << endl;
		//cout << "Read Calls   :" << read_calls << endl;
		//cout << "     Avg Time:" << read_avg_time << endl;
		//cout << "Getattr Calls:" << getattr_calls << endl;
		//cout << "     Avg Time:" << getattr_avg_time << endl;
		//cout << "Write Calls  :" << write_calls << endl;
		//cout << "     Avg Time:" << write_avg_time << endl;
		//cout << "Create Calls :" << create_calls << endl;
		//cout << "     Avg Time:" << create_avg_time << endl;
		//cout << "Rename Calls :" << rename_calls << endl;
		//cout << "     Avg Time:" << rename_avg_time << endl;
		vold_calls=0;
		readdir_calls=0;
		access_calls=0;
		getattr_calls=0;
		read_calls=0;
		write_calls=0;
		create_calls=0;
		rename_calls=0;
		tot_time=0;
		vold_avg_time=0;
		readdir_avg_time=0;
		access_avg_time=0;
		getattr_avg_time=0;
		read_avg_time=0;
		write_avg_time=0;
		create_avg_time=0;
		rename_avg_time=0;
		return -1;
	}

	//log_msg("xmp_mkdir");
	sprintf(msg,"khan_mkdir for path=%s\n",path);
	//log_msg(msg);
	struct stat *st;
	if(khan_getattr(path, st)<0) {
		//add path
		database_setval("alldirs","paths",path);
		//and break into attr/val pair and add to vold
	} else {
		//log_msg("Directory exists\n");
	}
	return 0;
}


static int xmp_readlink(const char *path, char *buf, size_t size) {
	//log_msg("xmp_readlink");
	//TODO: handle in vold somehow
	//log_msg("In readlink\n");
	int res = -1;
	path=append_path2(basename(strdup(path)));
	//res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;
	buf[res] = '\0';
	return 0;
}


static int xmp_unlink(const char *path) {
	//log_msg("in xmp_unlink");
	//TODO: handle in vold somehow
	int res;
	string fileid=database_getval("name",basename(strdup(path)));

	string fromext=database_getval(fileid,"ext");
	string file=append_path2(basename(strdup(path)));
	string attrs=database_getval(fromext,"attrs");
	//cout << fromext <<  fileid << endl;
	//cout<<"HERE!"<<endl;
	database_remove_val(fileid,"attrs","all_"+fromext+"s");
	//cout<<"THERE!"<<endl;
	//database_remove_val("all_"+fromext+"s",strdup(basename(strdup(from))),fileid);
	//cout<<"WHERE!"<<endl;
	string token="";
	stringstream ss2(attrs.c_str());
	while(getline(ss2,token,':')){
		if(strcmp(token.c_str(),"null")!=0){
			string cmd=database_getval(token+"gen","command");
			string msg2=(cmd+" "+file).c_str();
			FILE* stream=popen(msg2.c_str(),"r");
			if(fgets(msg,200,stream)!=0){
				database_remove_val(fileid,token,msg);
			}
			pclose(stream);
		}
	}

	path=append_path2(basename(strdup(path)));
	res = unlink(path);
	if (res == -1)
		return -errno;
	return 0;
}


static int xmp_rmdir(const char *path) {
	//if hardcoded, just remove
	database_remove_val("alldirs","paths",path);

	//if exists
	//get contained files
	//get attrs+vals from path
	//unset files attrs
	//if entire attr, remove attr

	return 0;
}

static int xmp_symlink(const char *from, const char *to) {
	//log_msg("in xmp_symlink");
	//TODO: handle in vold somehow
	int res=-1;
	from=append_path2(basename(strdup(from)));
	to=append_path2(basename(strdup(to)));
	sprintf(msg,"In symlink creating a symbolic link from %s to %s\n",from, to);
	//log_msg(msg);
	//res = symlink(from, to);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_rename(const char *from, const char *to) {
	//cout << endl << endl << endl << "Entering Rename Function" << endl;
	double start_time = 0;
	struct timeval start_tv;
	gettimeofday(&start_tv, NULL); 
	start_time = start_tv.tv_sec;
	start_time += (start_tv.tv_usec/1000000.0);
	start_times << fixed << start_time << endl << flush;
	string src = basename(strdup(from));
	string dst = basename(strdup(to));
	string fileid = database_getval("name", src);
	//cout << fileid << endl;
	database_remove_val(fileid,"name",src);
	//cout << src << endl;
	database_setval(fileid,"name",dst);
	//cout << dst << endl;
	string orig_path = append_path2(src);
	string orig_loc = database_getval(fileid,"location");
	map_path(resolve_selectors(to), fileid);
	string new_path = append_path2(dst);
	string new_loc = database_getval(fileid,"location");
	if(new_loc!=orig_loc) {
		if(new_loc=="google_music") {
			//upload
			//cloud_upload(orig_path);
		} else if(orig_loc=="google_music") {
			//download
			//cloud_download(src, new_path);
		} else {
			//file system rename
			rename(orig_path.c_str(), new_path.c_str());
		}
	}
	double rename_time = 0;
	struct timeval end_tv;
	gettimeofday(&end_tv, NULL); 
	rename_time = end_tv.tv_sec - start_tv.tv_sec;
	rename_time += (end_tv.tv_usec - start_tv.tv_usec) / 1000000.0;
	rename_times << fixed << rename_time << endl << flush;
	//cout << "Exiting Rename Function" << endl << endl << endl << endl;
	return 0;
}

static int xmp_link(const char *from, const char *to) {
	//log_msg("in xmp_link");
	//TODO:handle in vold somehow...
	int retstat = 0;
	from=append_path2(basename(strdup(from)));
	to=append_path2(basename(strdup(to)));
	sprintf(msg,"khan_link initial path=\"%s\", initial to=\"%s\")\n",from, to);
	//log_msg(msg);
	retstat = link(from,to);
	return retstat;
}

static int xmp_chmod(const char *path, mode_t mode) {
	//log_msg("in xmp_chmod");

	int res;
	path=append_path2(basename(strdup(path)));
	sprintf(msg, "In chmod for: %s\n",path);
	//log_msg(msg);
	res = chmod(path, mode);
#ifdef APPLE
	res = chmod(path, mode);
#else
	res = chmod(path, mode);
#endif
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid) {
	//log_msg("in xmp_chown");
	int res;
	path=append_path2(basename(strdup(path)));
	sprintf(msg,"In chown for : %s\n",path);
	//log_msg(msg);
	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_truncate(const char *path, off_t size) {
	//update for vold?
	//log_msg("In xmp_truncate\n");
	int res;
	path++;
	res = truncate(path, size);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2]) {
	//log_msg("in utimens");
	int res;
	struct timeval tv[2];
	path=append_path2(basename(strdup(path)));
	sprintf(msg,"in utimens for path : %s\n",path);
	//log_msg(msg);
	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;
	res = utimes(path, tv);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	int res = 0;
	path=append_path2(basename(strdup(path)));
	//cout<<"Converted Path: "<<path<<endl<<endl<<endl<<endl;

	FILE *thefile = fopen(path, "r");
	if (thefile != NULL){
		fseek(thefile, offset, SEEK_SET);
		res = fread(buf, 1, size, thefile);
		//cout << "READ THIS MANY"<<endl<<res<<endl<<endl<<endl<<endl<<endl;

		if (res == -1)
			res = -errno;
		fclose(thefile);
	} else {
		res = -errno;
	}
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	//log_msg("in xmp_write");
	int fd;
	int res;

	path=append_path2(basename(strdup(path)));
	(void) fi;
	fd = open(path, O_WRONLY);
	if (fd == -1){
		return errno;
	}
	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = errno;
	close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf) {
	/* Pass the call through to the underlying system which has the media. */
	int res = statvfs(path, stbuf);
	if (res != 0) {
		fprintf(log, "statfs error for %s\n",path);
		return errno;
	}
	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi) {
	/* Just a stub. This method is optional and can safely be left unimplemented. */
	fprintf(log, "in xmp_release with path %s\n", path);
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,struct fuse_file_info *fi) {
	/* Just a stub. This method is optional and can safely be left unimplemented. */
	fprintf(log, "in xmp_fsync with path %s\n", path);
	return 0;
}


void *khan_init(struct fuse_conn_info *conn) {

	//log_msg("khan_init() called!\n");
	sprintf(msg,"khan_root is : %s\n",servers.at(0).c_str());
	//log_msg(msg);
	if(chdir(servers.at(0).c_str())<0) {
		sprintf(msg,"could not change directory ,errno %s\n",strerror(errno)); 
		//log_msg(msg);
		perror(servers.at(0).c_str());
	}
	sprintf(msg,"AT THE END OF INIT\n"); 
	//log_msg(msg);
	return KHAN_DATA;
}



int khan_flush (const char * path, struct fuse_file_info * info ) {
	//cout << "=============IN KHAN FLUSH!!!!!!!!" << endl << endl;
	string filename = basename(strdup(path));
	string fileid=database_getval("name",filename);
	string server=database_getval(fileid,"server");
	process_file(server, fileid);
	return 0;
}


int khan_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{

	create_calls++;

	string fileid=database_getval("name",basename(strdup(path)));
	if(strcmp(fileid.c_str(),"null")==0){
		fileid=database_setval("null","name",basename(strdup(path)));
		database_setval(fileid, "server", servers.at(0));
		string ext = strrchr(basename(strdup(path)),'.')+1;
		database_setval(fileid, "ext", ext);
	}
	string server = database_getval(fileid, "server");

	process_file(server, fileid);

	map_path(resolve_selectors(path), fileid);

	return 0;
}

string bin2hex(const char* input, size_t size)
{
	std::string res;
	const char hex[] = "0123456789ABCDEF";
	for(int i=0; i<size; i++)
	{
		unsigned char c = input[i];
		res += (char)(c+10);
		//res += hex[c >> 4];
		//res += hex[c & 0xf];
	}

	return res;
}

string hex2bin(string in) {
	for(int i=0; i<in.length(); i++) {
		in[i]-=10;
	}
	return in;
}


int khan_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
	//log_msg("in khan_fgetattr");

	int retstat = 0;
	sprintf(msg,"khan_fgetattr(path=%s \n",path);
	retstat = fstat(fi->fh, statbuf);
	return retstat;
}
#ifdef APPLE
static int xmp_setxattr(const char *path, const char *name, const char *value,  size_t size, int flags, uint32_t param) {
#else
	static int xmp_setxattr(const char *path, const char *name, const char *value,  size_t size, int flags) {
#endif
		string attr = bin2hex(value, size);
		fprintf(stderr, "setxattr call\npath:%sname:%svalue:%s\n\n\n\n\n\n\n\n\n\n", path, name, value);
		string xpath = "xattr:";
		xpath += path;
		redis_setval(xpath, name, attr.c_str());
		fprintf(stderr, "setxattr call\n %s, %s, %s\n\n\n\n\n\n\n\n\n\n", xpath.c_str(), name, attr.c_str());
		return 0;
	}
#ifdef APPLE
	static int xmp_getxattr(const char *path, const char *name, char *value, size_t size, uint32_t param) {
#else
		static int xmp_getxattr(const char *path, const char *name, char *value, size_t size) {
#endif
			fprintf(stderr, "getxattr call\n %s, %s, %s\n", path, name, value);
			string xpath = "xattr:";
			xpath += path;
			string db_val = redis_getval( xpath, name);
			fprintf(stderr, "getxattr call\npath:%s\nname:%s\nvalue:%s\nsize:%zd\n\n\n\n\n", xpath.c_str(), name, db_val.c_str(), size);
			if(db_val != "null") {
				db_val = hex2bin(db_val);
				if(value==NULL) {
					errno = 0;
					return db_val.length();
				}
				memcpy(value, db_val.c_str(), size);
				size_t num = snprintf(value, size, "%s", db_val.c_str());
				fprintf(stderr, "returned\nstring:%s\ncount:%zd\n\n", value, num);
				errno = 0;
				return size;
			}
			errno = 1;
			return -1;
		}

		static int xmp_listxattr(const char *path, char *list, size_t size) {
			fprintf(stderr, "listxattr call\n %s, %s\n\n", path, list);
			string xpath = "xattr:";
			xpath += path;
			string attrs = database_getvals(xpath);
			char* mal = strdup(attrs.c_str());
			int count = 1;
			int str_size = strlen(mal);
			for(int i = 0; i<str_size; i++) {
				if(mal[i]==':') {
					mal[i]='\0';
					count += 1;
				}
			}
			if(list==NULL) {
				fprintf(stderr, "returning %d\n", str_size);
				return str_size;
			}
			snprintf(list, size, "%s", attrs.c_str());
			fprintf(stderr, "returning %s and %d\n", list, count);
			return count;
		}

		static int xmp_removexattr(const char *path, const char *name) {
			fprintf(stderr, "removexattr call\n %s, %s\n\n", path, name);
			return 0;
		}

#ifdef APPLE

		static int xmp_setvolname(const char* param) {
			fprintf(stderr, "apple function called setvolname\n");
			return 0;
		}

		static int xmp_exchange(const char* param1, const char* param2, unsigned long param3) {
			fprintf(stderr, "apple function called exchange\n");
			return 0;
		}

		static int xmp_getxtimes(const char* param1, struct timespec* param2, struct timespec* param3) {
			fprintf(stderr, "apple function called xtimes\n");
			return 0;
		}

		static int xmp_setbkuptime(const char* param1, const struct timespec* param2) {
			fprintf(stderr, "apple function called setbkuptimes\n");
			return 0;
		}

		static int xmp_setchgtime(const char* param1, const struct timespec* param2) {
			fprintf(stderr, "apple function called setchgtimes\n");
			return 0;
		}

		static int xmp_setcrtime(const char* param1, const struct timespec* param2) {
			fprintf(stderr, "apple function called setcrtimes\n");
			return 0;
		}

		static int xmp_chflags(const char* param1, uint32_t param2) {
			fprintf(stderr, "apple function called chflags\n");
			return 0;
		}
		static int xmp_setattr_x(const char* param1, struct setattr_x* param2) {
			fprintf(stderr, "apple function called setattr_x\n");
			return 0;
		}

		static int xmp_fsetattr_x(const char* param1, struct setattr_x* param2, struct fuse_file_info* param3) {
			fprintf(stderr, "apple function called fsetattr_x\n");
			return 0;
		}

#endif
		struct khan_param {
			unsigned                major;
			unsigned                minor;
			char                    *dev_name;
			int                     is_help;
		};

#define KHAN_OPT(t, p) { t, offsetof(struct khan_param, p), 1 }

		static const struct fuse_opt khan_opts[] = {
			KHAN_OPT("-s %s",            dev_name),
			KHAN_OPT("--cs %s",        dev_name),
			FUSE_OPT_END
		};

		static int khan_process_arg(void *data, const char *arg, int key, struct fuse_args *outargs) {
			struct khan_param *param = (struct khan_param*)data;
			fprintf(stderr,"param.dev_name : %s\n",param->dev_name);
			(void)outargs;
			(void)arg;
			switch (key) {
				case 0:
					param->is_help = 1;
					return fuse_opt_add_arg(outargs, "-ho");
				default:
					return 1;
			}
		}

		static struct fuse_operations khan_ops;


		void my_terminate(int param) {
			chdir("/Users/drewbratcher/projects/mediakhan/");
			cout << "killing... " << flush << endl;
			exit(1);
		}

		int main(int argc, char *argv[])
		{
			khan_ops.getattr  = khan_getattr;
			khan_ops.init     = khan_init;
			khan_ops.access    = xmp_access;
			khan_ops.readlink  = xmp_readlink;
			khan_ops.readdir  = xmp_readdir;
			khan_ops.mknod    = xmp_mknod;
			khan_ops.mkdir    = xmp_mkdir;
			khan_ops.symlink  = xmp_symlink;
			khan_ops.unlink    = xmp_unlink;
			khan_ops.rmdir    = xmp_rmdir;
			khan_ops.rename    = xmp_rename;
			khan_ops.link    = xmp_link;
			khan_ops.chmod    = xmp_chmod;
			khan_ops.chown    = xmp_chown;
			khan_ops.truncate  = xmp_truncate;
			khan_ops.create   = khan_create;
			khan_ops.utimens  = xmp_utimens;
			khan_ops.open    = khan_open;
			khan_ops.read    = xmp_read;
			khan_ops.write    = xmp_write;
			khan_ops.statfs    = xmp_statfs;
			khan_ops.release  = xmp_release;
			khan_ops.fsync    = xmp_fsync;
			khan_ops.opendir  = khan_opendir;
			khan_ops.flush    = khan_flush;
			khan_ops.getxattr  = xmp_getxattr;
#ifdef APPLE
			khan_ops.setxattr  = xmp_setxattr;
			khan_ops.listxattr  = xmp_listxattr;
			khan_ops.removexattr  = xmp_removexattr;
			khan_ops.setvolname     = xmp_setvolname;
			khan_ops.exchange       = xmp_exchange;
			khan_ops.getxtimes      = xmp_getxtimes;
			khan_ops.setbkuptime    = xmp_setbkuptime;
			khan_ops.setchgtime     = xmp_setchgtime;
			khan_ops.setcrtime      = xmp_setcrtime;
			khan_ops.chflags        = xmp_chflags;
			khan_ops.setattr_x      = xmp_setattr_x;
			khan_ops.fsetattr_x     = xmp_fsetattr_x;
#endif

//			Py_SetProgramName(argv[0]);  /* optional but recommended */
//			Py_Initialize();
//			PyObject *sys = PyImport_ImportModule("sys");
//			PyObject *path = PyObject_GetAttrString(sys, "path");
//			PyList_Append(path, PyString_FromString("."));

			int retval=0;
			struct khan_param param = { 0, 0, NULL, 0 };
			if((argc<2)||(argc>4)) {
				printf("Usage: ./khan <mount_dir_location> [stores.txt] [-d]\nAborting...\n");
				exit(1);
			}

			struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
			int j;
			const char* store_filename="stores.txt";
			for(j = 0; j < argc; j++) {
				if((j == 2) && (argv[j][0]!='-')) {
					store_filename = argv[j];
				} else {
					fuse_opt_add_arg(&args, argv[j]);
				}
			}

			//set signal handler
			signal(SIGTERM, my_terminate);
			signal(SIGKILL, my_terminate);

			fprintf(stderr, "store filename: %s\n", store_filename);
			FILE* stores = fopen(store_filename, "r");
			char buffer[100];
			char buffer2[100];
			fscanf(stores, "%s\n", buffer);
			this_server_id = buffer;
			while(fscanf(stores, "%s %s\n", buffer, buffer2)!=EOF) {
				if(strcmp(buffer,"cloud")==0) {
					string module = buffer2;
					module = "cloud." + module;
					//cloud_interface = PyImport_ImportModule(module.c_str());
				}
				servers.push_back(buffer);
				server_ids.push_back(buffer2);
				if(this_server_id == buffer2) {
					this_server = buffer;
				}
			}
			fclose(stores);
			umask(0);
			if(-1==log_open()) {
				printf("Unable to open the log file..NO log would be recorded..!\n");
			}
			//log_msg("\n\n--------------------------------------------------------\n");
			khan_data = (khan_state*)calloc(sizeof(struct khan_state), 1);
			if (khan_data == NULL)  {
				//log_msg("Could not allocate memory to khan_data!..Aborting..!\n");
				abort();
			}
			if(initializing_khan(argv[1])<0)  {
				//log_msg("Could not initialize khan..Aborting..!\n");
				return -1;
			}
			rename_times.open(rename_times_file_name.c_str(), ofstream::out);
			rename_times.precision(15);
			start_times.open(start_times_file_name.c_str(), ofstream::out);
			start_times.precision(15);
			//log_msg("initialized....");
			retval=fuse_main(args.argc,args.argv, &khan_ops, khan_data);
			//log_msg("Done with fuse_main...\n");
			return retval;
		}

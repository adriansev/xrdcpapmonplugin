
  
// This file implements an instance of the XrdClientAbsMonIntf abstract class,
//  to be used to send data from xrdcp to Monalisa in the Alice environment

#include <errno.h>
#include <iostream>

#include "XrdClient/XrdClientAbsMonIntf.hh"
#include "ApMon.h"


#define MIN_SEND_INTERVAL 10.0                // don't send data more often that this


class XrdCpapMon : public XrdClientAbsMonIntf
{

public:
  // Initialization of the external library
  virtual int Init(const char *src, const char *dest, int debug=0, void *parm=0);
  virtual int DeInit();

  // To get the name of the library and other info
  // It could point to static strings
  virtual int GetMonLibInfo(char **name, char **version, char **remarks);


  // To submit a set of info about the progress of something
  // Set force to true to be sure that the info is sent and not eventually skipped
  virtual int PutProgressInfo(long long bytecount=0,
			      long long size=0,
			      float percentage=0.0,
			      bool force=false);


  XrdCpapMon();
  virtual ~XrdCpapMon();

private:

  // ApMon Monitoring
  bool    initdone;
  ApMon  *apmon;
  int     apmon_nparams;
  char**  apmon_params;
  int*    apmon_types;
  char**  apmon_values;
  char*   job_id;
  char   *host_name;
  char    statichostname[256];
  bool    is_src_remote;
  bool    is_dst_remote;
  double  src_IP;
  double  dst_IP;
  struct timeval last_send_time, abs_start_time;


};
 
/******************************************************************************/
/*                        I m p l e m e n t a t i o n                         */
/******************************************************************************/

  
XrdCpapMon::XrdCpapMon():
  initdone(false), apmon(0), apmon_nparams(0), apmon_params(0),
  apmon_types(0), apmon_values(0), job_id(0), host_name(0), is_src_remote(false),
  is_dst_remote(false), src_IP(0), dst_IP(0)
{
 

}


XrdCpapMon::~XrdCpapMon() {
  if (initdone) DeInit();
}


int XrdCpapMon::Init(const char *src, const char *dest, int debug, void *parm) {

  // If ApMon is enabled, initialize it
  struct timezone tz;
  gettimeofday(&abs_start_time,&tz);

  char * apmon_config[1]={ getenv("APMON_CONFIG") };
  if ( (apmon_config[0] == 0) || (!strlen(apmon_config[0])) ) {
    cerr << "Disabling apmon monitoring since env variable APMON_CONFIG was not found" << std::endl;
    return 1;
  }
  try {

    if (!debug)
       ApMon::setLogLevel((char *)"WARNING");
    else
      ApMon::setLogLevel((char *)"DEBUG"); // DEBUG or WARNING

    apmon = new ApMon(1, apmon_config);
    apmon->setConfRecheck(false);
    apmon->setJobMonitoring(false);
    //apmon->setSysMonitoring(false);
    //apmon->setGenMonitoring(false);

  } catch(runtime_error &e){

    cerr << "Error initializing ApMon: " << e.what() << std::endl;
    cerr << "Disabling apmon." << std::endl;
    apmon = 0;

    return 1;

  }

  if (!apmon) {
     cerr << "Error initializing ApMon." << std::endl;
     return 1;
  }


  is_src_remote = strstr(src, "root://") == src;
  is_dst_remote = strstr(dest, "root://") == dest;

  double local_IP = src_IP = dst_IP = 0;
  host_name = getenv("HOSTNAME");
  if(host_name == NULL)
    host_name = getenv("HOST");

   if(host_name == NULL){
    cerr << "Error getting my hostname from $HOSTNAME or $HOST. Taking the machine's name, please verify if this is good or not." << std::endl;
    
    if (gethostname(statichostname, sizeof(statichostname))) {
       cerr << "Error calling gethostname ?!?!?" << std::endl;
       return 2;
    }

    statichostname[sizeof(statichostname)-1] = 0;
    host_name = statichostname;
    cerr << "The machine's hostname is '" << host_name << "'" << std::endl;
  }
 
  if(host_name == NULL){
    cerr << "Error getting my hostname. Disabling ApMon." << std::endl;
    
    return 2;
  }

  struct hostent * he = gethostbyname(host_name);
  if(he != NULL)
    local_IP = ((struct in_addr *)he->h_addr)->s_addr;

  char *host;
  if(is_src_remote) {
    int len = 0;
    for(const char *p = src+7; *p && *p != ':' && *p != '/' ; p++, len++);
    host = (char *) calloc(len+1, 1);
    strncpy(host, src+7, len);
    he = gethostbyname(host);
    free(host);
    if(he != NULL)
      src_IP = ((struct in_addr *)he->h_addr)->s_addr;
    dst_IP = local_IP;
  }

  if(is_dst_remote) {
    int len = 0;
    for(const char *p = dest+7; *p && *p != ':' && *p != '/'; p++, len++);
    host = (char *) calloc(len+1, 1);
    strncpy(host, dest+7, len);
    he = gethostbyname(host);
    free(host);
    if(he != NULL)
      dst_IP = ((struct in_addr *)he->h_addr)->s_addr;
    src_IP = local_IP;
  }

  if (getenv("ALIEN_PROC_ID"))
    job_id = getenv("ALIEN_PROC_ID");
  else
    if (getenv("GRID_JOB_ID"))
      job_id = getenv("GRID_JOB_ID");

  apmon_nparams = 9;
  apmon_params = (char **)malloc(apmon_nparams * sizeof(char*));
  apmon_values = (char **)malloc(apmon_nparams * sizeof(char*));
  apmon_types = (int *)malloc(apmon_nparams * sizeof(int));
  apmon_params[0] = (char *)"total_size";
  apmon_types[0] = XDR_REAL64;
  apmon_params[1] = (char *)"moved_bytes";
  apmon_types[1] = XDR_REAL64;
  apmon_params[2] = (char *)"read_bytes";
  apmon_types[2] = XDR_REAL64;
  apmon_params[3] = (char *)"written_bytes";
  apmon_types[3] = XDR_REAL64;
  apmon_params[4] = (char *)"speed";
  apmon_types[4] = XDR_REAL64;
  apmon_params[5] = (char *)"elapsed_time";
  apmon_types[5] = XDR_REAL64;
  apmon_params[6] = (char *)"percent";
  apmon_types[6] = XDR_REAL64;
  apmon_params[7] = (char *)"src_IP";
  apmon_types[7] = XDR_REAL64;
  apmon_params[8] = (char *)"dst_IP";
  apmon_types[8] = XDR_REAL64;



  return 0;

};

int XrdCpapMon::DeInit() {

  delete apmon;
  apmon = 0;

  delete apmon_params;
  delete apmon_values;
  delete apmon_types;

  apmon_params = 0;
  apmon_values = 0;
  apmon_types = 0;
};


const char *libname = "XrdCpApMon shared lib";
const char *libversion = "1.0";
const char *libremarks = "";

int XrdCpapMon::GetMonLibInfo(char **name, char **version, char **remarks) {
   *name = const_cast<char *>(libname);
   *version = const_cast<char *>(libversion);
   *remarks = const_cast<char *>(libremarks);

  return 0;
};


int XrdCpapMon::PutProgressInfo(long long bytecount,
				long long size,
				float percentage,
				bool force) {

  struct timeval timenow;
  struct timezone tz;
  gettimeofday (&timenow, &tz);

  if (!apmon || !apmon_params || !apmon_values || !apmon_types)
     return 1;

  if(!force){
    float abs_time=((float)((timenow.tv_sec - last_send_time.tv_sec) +
			    (timenow.tv_usec - last_send_time.tv_usec) / 1000000));
    if(abs_time < MIN_SEND_INTERVAL)
      return 0;
    last_send_time = timenow;
  }

  float abs_time=((float)((timenow.tv_sec - abs_start_time.tv_sec) *1000 +
			  (timenow.tv_usec - abs_start_time.tv_usec) / 1000));

  try{
    double total_size = size;
    double moved_bytes = bytecount;
    double read_bytes = is_src_remote ? bytecount : 0;
    double written_bytes = is_dst_remote ? bytecount : 0;
    double speed = bytecount/abs_time/1000.0;
    double elapsed_time = abs_time;
    double percent = percentage;
    
    apmon_values[0] = (char *)&total_size;   // total size of this transfer, in bytes
    apmon_values[1] = (char *)&moved_bytes;  // number of bytes moved so far
    apmon_values[2] = (char *)&read_bytes;   // number of read bytes
    apmon_values[3] = (char *)&written_bytes;// number of written bytes
    apmon_values[4] = (char *)&speed;        // transfer speed, in Mb/s
    apmon_values[5] = (char *)&elapsed_time; // elapsed time, in milliseconds, since start of this transfer
    apmon_values[6] = (char *)&percent;      // percent of the completed transfer
    apmon_values[7] = (char *)&src_IP;       // source IP, as a double value
    apmon_values[8] = (char *)&dst_IP;       // destination IP, as a double value

    if(job_id == NULL)
       apmon->sendParameters(const_cast<char *>("Other_XRD_Transfers"), host_name, apmon_nparams,
			    apmon_params, apmon_types, apmon_values);
    else
       apmon->sendParameters(const_cast<char *>("Job_XRD_Transfers"), job_id, apmon_nparams,
			    apmon_params, apmon_types, apmon_values);
  }catch(runtime_error &e){
    cerr << "[ApMon] Error sending parameters" << std::endl;
    return 1;
  }


  return 0;

};



/******************************************************************************/
/*                    X r d C l i e n t g e t M o n I n t f                   */
/******************************************************************************/
  
XrdClientAbsMonIntf *XrdClientgetMonIntf(XrdClientMonIntfArgs)
{
   XrdCpapMon *inst = new XrdCpapMon();

   return (XrdClientAbsMonIntf *)inst;
}

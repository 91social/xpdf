//========================================================================
//
// pdftohtml.cc
//
// Copyright 2005 Glyph & Cog, LLC
//
//========================================================================

#include <iostream>
#include <memory>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <aconf.h>
#include <stdio.h>
#include <stdlib.h>
#include "gmem.h"
#include "gmempp.h"
#include "parseargs.h"
#include "gfile.h"
#include "GString.h"
#include "GlobalParams.h"
#include "PDFDoc.h"
#include "HTMLGen.h"
#include "Error.h"
#include "ErrorCodes.h"
#include "config.h"
#include <string>


#include "src/main/com/datasahi/pdftohtml/grpc/pdftohtml.grpc.pb.h"

//------------------------------------------------------------------------

static GBool createIndex(char *htmlDir);

//------------------------------------------------------------------------

static int firstPage = 1;
static int lastPage = 0;
static double zoom = 1;
static int resolution = 150;
static GBool skipInvisible = gFalse;
static GBool skipImages = gFalse;
static GBool allInvisible = gFalse;
static char ownerPassword[33] = "\001";
static char port[32] = "\001";
static char userPassword[33] = "\001";
static GBool quiet = gFalse;
static char cfgFileName[256] = "";
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;

static ArgDesc argDesc[] = {
  {"-server",       argString,   port, sizeof(port),
   "for port number"},
  {"-f",       argInt,      &firstPage,     0,
   "first page to convert"},
  {"-l",       argInt,      &lastPage,      0,
   "last page to convert"},
  {"-z",      argFP,       &zoom,     0,
   "initial zoom level (1.0 means 72dpi)"},
  {"-r",      argInt,      &resolution,     0,
   "resolution, in DPI (default is 150)"},
  {"-skipinvisible", argFlag, &skipInvisible, 0,
   "do not draw invisible text"},
  {"-skipimages", argFlag, &skipImages, 0,
   "skip images"},
  {"-allinvisible",  argFlag, &allInvisible,  0,
   "treat all text as invisible"},
  {"-opw",     argString,   ownerPassword,  sizeof(ownerPassword),
   "owner password (for encrypted files)"},
  {"-upw",     argString,   userPassword,   sizeof(userPassword),
   "user password (for encrypted files)"},
  {"-q",       argFlag,     &quiet,         0,
   "don't print any messages or errors"},
  {"-cfg",     argString,   cfgFileName,    sizeof(cfgFileName),
   "configuration file to use in place of .xpdfrc"},
  {"-v",       argFlag,     &printVersion,  0,
   "print copyright and version info"},
  {"-h",       argFlag,     &printHelp,     0,
   "print usage information"},
  {"-help",    argFlag,     &printHelp,     0,
   "print usage information"},
  {"--help",   argFlag,     &printHelp,     0,
   "print usage information"},
  {"-?",       argFlag,     &printHelp,     0,
   "print usage information"},
  {NULL}
};

//------------------------------------------------------------------------


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using pdftohtml::Reply;
using pdftohtml::Request;
using pdftohtml::ConvertPdfToHtml;

using namespace std;

int convert(int argc, char *argv[]);

static int writeToFile(void *file, const char *data, int size) {
  return (int)fwrite(data, 1, size, (FILE *)file);
}


class PdfConvertServiceImpl final : public ConvertPdfToHtml::Service {
  Status PdfToHtml(ServerContext* context, const Request* request,
                  Reply* reply) override {

std::cout << "Service started  " << std::endl;
      char *parameters[3];
      string str = request->source();
      parameters[1] = &str[0];
      string str1 = request->destination();
      parameters[2] = &str1[0];
      string str2 = request->password();
      parameters[0] = &str2[0];


      int ret = convert(3, parameters);
      switch(ret)
      {
      case 0:
      reply->set_status("successfull");
      return Status::OK;

      case 1:
      reply->set_status("unsuccessfull: problem reading pdf document please check password and permission");
      return Status::OK;

      case 2:
      reply->set_status("unsuccessfull: Couldn't create HTML output directory");
      return Status::OK;

      case 3:
      reply->set_status("unsuccessfull: Copying of text from this document is not allowed");
      return Status::OK;

      case 99:
      reply->set_status("unsuccessfull: problem generating html");
      return Status::OK;

      default:
      reply->set_status("unsuccessfull");
      return Status::OK;

      }
  }
};


void RunServer(char* port) {
  std::string server_address(port);
  PdfConvertServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}


int main(int argc, char *argv[]) {

 fixCommandLine(&argc, &argv);
  GBool ok = parseArgs(argDesc, &argc, argv);
  if (!ok || printVersion || printHelp) {
    fprintf(stderr, "pdftohtml version %s\n", xpdfVersion);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (!printVersion) {
      printUsage("pdftohtml", "<PDF-file> <html-dir>", argDesc);
    }

  }

    if(port[0]!='\001')
    {
          RunServer(port);
    }
    else {
        return convert(argc,argv);
    }
}

int convert(int argc, char *argv[]){
  PDFDoc *doc;
  char *fileName;
  char *htmlDir;
  GString *ownerPW, *userPW;
  HTMLGen *htmlGen;
  GString *htmlFileName, *pngFileName, *pngURL;
  FILE *htmlFile, *pngFile;
  int pg, err, exitCode;
  GBool ok;

  exitCode = 99;

  // parse args
  fixCommandLine(&argc, &argv);
  ok = parseArgs(argDesc, &argc, argv);
  if (!ok || argc != 3 || printVersion || printHelp) {
    fprintf(stderr, "pdftohtml version %s\n", xpdfVersion);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (!printVersion) {
      printUsage("pdftohtml", "<PDF-file> <html-dir>", argDesc);
    }
    goto err0;
  }
  fileName = argv[1];
  htmlDir = argv[2];

if(port[0]!='\001')
{
  strcpy(ownerPassword,argv[0]);
}
//std::cout<<"file name is: " << fileName << "\t  directory is"  << htmlDir <<"\t password is " << ownerPassword << std::endl ;
  // read config file
  globalParams = new GlobalParams(cfgFileName);
  if (quiet) {
    globalParams->setErrQuiet(quiet);
  }
  globalParams->setupBaseFonts(NULL);
  globalParams->setTextEncoding("UTF-8");


  // open PDF file
  if (ownerPassword[0] != '\001') {
    ownerPW = new GString(ownerPassword);
  } else {
    ownerPW = NULL;
  }
  if (userPassword[0] != '\001') {
    userPW = new GString(userPassword);
  } else {
    userPW = NULL;
  }

  doc = new PDFDoc(fileName, ownerPW, userPW);
  if (userPW) {
    delete userPW;
  }
  if (ownerPW) {
    delete ownerPW;
  }
  if (!doc->isOk()) {
    exitCode = 1;
    goto err1;
  }

  // check for copy permission
  if (!doc->okToCopy()) {
    error(errNotAllowed, -1,
	  "Copying of text from this document is not allowed.");
    exitCode = 3;
    goto err1;
  }

  // get page range
  if (firstPage < 1) {
    firstPage = 1;
  }
  if (lastPage < 1 || lastPage > doc->getNumPages()) {
    lastPage = doc->getNumPages();
  }

  // create HTML directory
  if (!createDir(htmlDir, 0755)) {
    error(errIO, -1, "Couldn't create HTML output directory '{0:s}'",
	  htmlDir);
    exitCode = 2;
    goto err1;
  }

  // set up the HTMLGen object
  htmlGen = new HTMLGen(resolution);
  if (!htmlGen->isOk()) {
    exitCode = 99;
    goto err1;
  }
  htmlGen->setZoom(zoom);
  htmlGen->setDrawInvisibleText(!skipInvisible);
  htmlGen->setSkipImages(skipImages);
  htmlGen->setAllTextInvisible(allInvisible);
  htmlGen->setExtractFontFiles(gTrue);
  htmlGen->startDoc(doc);


  // convert the pages
  for (pg = firstPage; pg <= lastPage; ++pg) {
    htmlFileName = GString::format("{0:s}/page{1:d}.html", htmlDir, pg);
    pngFileName = GString::format("{0:s}/page{1:d}.png", htmlDir, pg);

    if (!(htmlFile = fopen(htmlFileName->getCString(), "wb"))) {
      error(errIO, -1, "Couldn't open HTML file '{0:t}'", htmlFileName);
      delete htmlFileName;
      delete pngFileName;
      goto err2;
    }
    if (!skipImages) {
        if (!(pngFile = fopen(pngFileName->getCString(), "wb"))) {
            error(errIO, -1, "Couldn't open PNG file '{0:t}'", pngFileName);
            fclose(htmlFile);
            delete htmlFileName;
            delete pngFileName;
            goto err2;
        }
    }

    pngURL = GString::format("page{0:d}.png", pg);
    err = htmlGen->convertPage(pg, pngURL->getCString(), htmlDir,
			       &writeToFile, htmlFile,
			       &writeToFile, pngFile);
    delete pngURL;
    fclose(htmlFile);
    if (!skipImages) fclose(pngFile);
    delete htmlFileName;
    delete pngFileName;
    if (err != errNone) {
      error(errIO, -1, "Error converting page {0:d}", pg);
      exitCode = 2;
      goto err2;
    }
  }

  // create the master index
  if (!createIndex(htmlDir)) {
    exitCode = 2;
    goto err2;
  }

  exitCode = 0;

  // clean up
 err2:
  delete htmlGen;
 err1:
  delete doc;
  delete globalParams;
 err0:

  // check for memory leaks
  Object::memCheck(stderr);
  gMemReport(stderr);


  return exitCode;
}

static GBool createIndex(char *htmlDir) {
  GString *htmlFileName;
  FILE *html;
  int pg;

  htmlFileName = GString::format("{0:s}/index.html", htmlDir);
  html = fopen(htmlFileName->getCString(), "w");
  delete htmlFileName;
  if (!html) {
    error(errIO, -1, "Couldn't open HTML file '{0:t}'", htmlFileName);
    return gFalse;
  }

  fprintf(html, "<html>\n");
  fprintf(html, "<body>\n");
  for (pg = firstPage; pg <= lastPage; ++pg) {
    fprintf(html, "<a href=\"page%d.html\">page %d</a><br>\n", pg, pg);
  }
  fprintf(html, "</body>\n");
  fprintf(html, "</html>\n");

  fclose(html);

  return gTrue;
}

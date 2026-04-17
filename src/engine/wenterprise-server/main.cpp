// OES Web Runtime — standalone HTTP server
//
// Step 1 skeleton: routes /w/<db>/ and /w/<db>/ping so the plumbing can
// be validated before the real runtime (sessions, module manager, main
// form) is wired in.

#include <cstdlib>
#include <iostream>
#include <string>

#include "../../3rdparty/cpp-httplib/httplib.h"

namespace {

struct CmdArgs {
	std::string dbName = "default";
	int port = 8080;
	std::string host = "0.0.0.0";
};

CmdArgs ParseArgs(int argc, char** argv)
{
	CmdArgs a;
	for (int i = 1; i < argc; ++i) {
		const std::string arg(argv[i]);
		if      (arg.rfind("--db=",   0) == 0) a.dbName = arg.substr(5);
		else if (arg.rfind("--port=", 0) == 0) a.port   = std::atoi(arg.substr(7).c_str());
		else if (arg.rfind("--host=", 0) == 0) a.host   = arg.substr(7);
		else if (arg == "--help" || arg == "-h") {
			std::cout << "Usage: wenterprise-server [--db=<name>] [--port=<N>] [--host=<addr>]\n";
			std::exit(0);
		}
	}
	return a;
}

} // namespace

int main(int argc, char** argv)
{
	const CmdArgs args = ParseArgs(argc, argv);
	const std::string prefix = "/w/" + args.dbName;

	httplib::Server svr;

	svr.Get(prefix + "/", [&args](const httplib::Request&, httplib::Response& res) {
		res.set_content(
			"OES Web Runtime\nDatabase: " + args.dbName + "\n",
			"text/plain; charset=utf-8");
	});

	svr.Get(prefix + "/ping", [](const httplib::Request&, httplib::Response& res) {
		res.set_content("pong", "text/plain");
	});

	std::cout << "OES wenterprise-server listening on http://"
		<< args.host << ":" << args.port << prefix << "/" << std::endl;

	if (!svr.listen(args.host.c_str(), args.port)) {
		std::cerr << "Failed to bind " << args.host << ":" << args.port << std::endl;
		return 1;
	}
	return 0;
}

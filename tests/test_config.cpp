#include "../include/Webserv.hpp"
#include "../include/libraryHeader.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>

static int g_passed = 0;
static int g_failed = 0;
//from main.cpp
volatile sig_atomic_t	g_signum = 0;
int						g_sigPipe[2];

std::string	findExeRoot(void);

std::string exeRoot = findExeRoot();

std::string	findExeRoot(void)
{
	char	path[PATH_MAX];
	ssize_t	length = readlink("/proc/self/exe", path, sizeof(path)); //finds path of executable;
	if (length == -1)
		return ("");
	std::string	execPath(path, length);
	std::string::iterator end = execPath.begin() + execPath.find_last_of("/") + 1;
	return (std::string(execPath.begin(), end));
}

//takes path in root like /data/www/... and build the absolute path compared to actual execLocation
std::string	resolvePath(std::string relPath)
{
	if (relPath[0] == '/')
		relPath = &relPath[1];
	std::string	resultingPath = exeRoot + relPath;
	return (resultingPath);
}

void	signalHandler(int signum)
{
	if (signum == SIGINT)
	{
		g_signum = SIGINT;
		write(g_sigPipe[1], "a", 1);
	}
	return ;
}

// Helper to write a temp config file
static void writeConfig(const char *path, const char *content)
{
	std::ofstream ofs(path);
	ofs << content;
	ofs.close();
}

// Test that parsing succeeds (no exception) and print the result
static bool expectSuccess(const char *testName, const char *config)
{
	writeConfig("/tmp/test.conf", config);
	Webserv webserv;
	try
	{
		webserv.getConfig("/tmp/test.conf");
		std::cout << "[PASS] " << testName << std::endl;
		std::cout << "--- Parsed config ---" << std::endl;
		std::cout << webserv << std::endl;
		g_passed++;
		return true;
	}
	catch (const std::exception &e)
	{
		std::cout << "[FAIL] " << testName << " - unexpected exception: " << e.what() << std::endl;
		g_failed++;
		return false;
	}
}

// Test that parsing throws an exception
static bool expectException(const char *testName, const char *config)
{
	writeConfig("/tmp/test.conf", config);
	Webserv webserv;
	try
	{
		webserv.getConfig("/tmp/test.conf");
		std::cout << "[FAIL] " << testName << " - expected exception but none thrown" << std::endl;
		std::cout << "--- Parsed config (should have failed) ---" << std::endl;
		std::cout << webserv << std::endl;
		g_failed++;
		return false;
	}
	catch (const std::exception &e)
	{
		std::cout << "[PASS] " << testName << " - caught: " << e.what() << std::endl;
		g_passed++;
		return true;
	}
}

// ============== VALID CONFIG TESTS ==============

static void test_minimal_config()
{
	expectSuccess("minimal_config",
		"server { listen 8080; }");
}

static void test_full_config()
{
	expectSuccess("full_config",
		"server {"
		"  listen 127.0.0.1/8080;"
		"  server_name webserv;"
		"  client_max_body_size 1024;"
		"  root /var/www;"
		"  location / {"
		"    root /var/www/html;"
		"    limit_except GET, POST;"
		"    autoindex on;"
		"    index index.html;"
		"    upload_store /uploads;"
		"    error_page 404 /404.html;"
		"    return 301 http://example.com;"
		"  }"
		"}");
}

static void test_multiple_servers()
{
	expectSuccess("multiple_servers",
		"server { listen 8080; }"
		"server { listen 8081; }");
}

static void test_multiple_locations()
{
	expectSuccess("multiple_locations",
		"server {"
		"  listen 8080;"
		"  location /api/ { root /var/api; }"
		"  location /static/ { root /var/static; }"
		"}");
}

static void test_listen_port_only()
{
	expectSuccess("listen_port_only",
		"server { listen 9000; }");
}

static void test_multiple_listen()
{
	expectSuccess("multiple_listen",
		"server {"
		"  listen 8080;"
		"  listen 127.0.0.1/8081;"
		"}");
}

static void test_all_methods()
{
	expectSuccess("all_methods",
		"server {"
		"  listen 8080;"
		"  location / { limit_except GET, POST, DELETE; }"
		"}");
}

static void test_autoindex_off()
{
	expectSuccess("autoindex_off",
		"server {"
		"  listen 8080;"
		"  location / { autoindex off; }"
		"}");
}

static void test_error_page_range()
{
	expectSuccess("error_page_range",
		"server {"
		"  listen 8080;"
		"  location / {"
		"    error_page 400 /400.html;"
		"  }"
		"}");
}

static void test_return_range()
{
	expectSuccess("return_range",
		"server {"
		"  listen 8080;"
		"  location / { return 300 http://a.com; }"
		"}"
		"server {"
		"  listen 8081;"
		"  location / { return 399 http://b.com; }"
		"}");
}

static void test_single_method_get()
{
	expectSuccess("single_method_get",
		"server {"
		"  listen 8080;"
		"  location / { limit_except GET; }"
		"}");
}

static void test_single_method_post()
{
	expectSuccess("single_method_post",
		"server {"
		"  listen 8080;"
		"  location / { limit_except POST; }"
		"}");
}

static void test_single_method_delete()
{
	expectSuccess("single_method_delete",
		"server {"
		"  listen 8080;"
		"  location / { limit_except DELETE; }"
		"}");
}

static void test_multiple_error_pages()
{
	expectSuccess("multiple_error_pages",
		"server {"
		"  listen 8080;"
		"  location / {"
		"    error_page 400 /400.html;"
		"    error_page 403 /403.html;"
		"    error_page 404 /404.html;"
		"    error_page 405 /405.html;"
		"  }"
		"}");
}

static void test_nested_path()
{
	expectSuccess("nested_path",
		"server {"
		"  listen 8080;"
		"  location /a/b/c/d/e/ { root /deep/path; }"
		"}");
}

// ============== SYNTAX ERROR TESTS ==============

static void test_missing_server_open_brace()
{
	expectException("missing_server_open_brace",
		"server listen 8080; }");
}

static void test_missing_close_brace()
{
	expectException("missing_close_brace",
		"server { listen 8080;");
}

static void test_missing_semicolon()
{
	expectException("missing_semicolon",
		"server { listen 8080 }");
}

static void test_empty_file()
{
	expectException("empty_file", "");
}

static void test_no_server_block()
{
	expectException("no_server_block",
		"listen 8080;");
}

static void test_location_missing_open_brace()
{
	expectException("location_missing_open_brace",
		"server { listen 8080; location / root /var; }");
}

static void test_double_open_brace()
{
	expectException("double_open_brace",
		"server {{ listen 8080; }");
}

static void test_only_braces()
{
	expectException("only_braces",
		"{ }");
}

static void test_server_no_content()
{
	expectException("server_no_content",
		"server { }");
}

static void test_location_missing_close_brace()
{
	expectException("location_missing_close_brace",
		"server { listen 8080; location / { root /var; }");
}

// ============== INVALID VALUE TESTS ==============

static void test_listen_no_value()
{
	expectException("listen_no_value",
		"server { listen ; }");
}

static void test_server_name_no_value()
{
	expectException("server_name_no_value",
		"server { listen 8080; server_name ; }");
}

static void test_body_size_non_numeric()
{
	expectException("body_size_non_numeric",
		"server { listen 8080; client_max_body_size abc; }");
}

static void test_body_size_negative()
{
	expectException("body_size_negative",
		"server { listen 8080; client_max_body_size -100; }");
}

static void test_body_size_float()
{
	expectException("body_size_float",
		"server { listen 8080; client_max_body_size 10.5; }");
}

static void test_body_size_with_suffix()
{
	expectException("body_size_with_suffix",
		"server { listen 8080; client_max_body_size 10M; }");
}

static void test_root_no_slash()
{
	expectException("root_no_slash",
		"server { listen 8080; root relative/path; }");
}

static void test_root_empty()
{
	expectException("root_empty",
		"server { listen 8080; root ; }");
}

static void test_location_root_no_slash()
{
	expectException("location_root_no_slash",
		"server { listen 8080; location / { root relative; } }");
}

static void test_unknown_method()
{
	expectException("unknown_method",
		"server { listen 8080; location / { limit_except PUT; } }");
}

static void test_unknown_method_patch()
{
	expectException("unknown_method_patch",
		"server { listen 8080; location / { limit_except PATCH; } }");
}

static void test_unknown_method_head()
{
	expectException("unknown_method_head",
		"server { listen 8080; location / { limit_except HEAD; } }");
}

static void test_unknown_method_options()
{
	expectException("unknown_method_options",
		"server { listen 8080; location / { limit_except OPTIONS; } }");
}

static void test_method_lowercase()
{
	expectException("method_lowercase",
		"server { listen 8080; location / { limit_except get; } }");
}

static void test_method_mixed_case()
{
	expectException("method_mixed_case",
		"server { listen 8080; location / { limit_except Get; } }");
}

static void test_limit_except_no_value()
{
	expectException("limit_except_no_value",
		"server { listen 8080; location / { limit_except ; } }");
}

static void test_autoindex_invalid()
{
	expectException("autoindex_invalid",
		"server { listen 8080; location / { autoindex yes; } }");
}

static void test_autoindex_true()
{
	expectException("autoindex_true",
		"server { listen 8080; location / { autoindex true; } }");
}

static void test_autoindex_1()
{
	expectException("autoindex_1",
		"server { listen 8080; location / { autoindex 1; } }");
}

static void test_autoindex_empty()
{
	expectException("autoindex_empty",
		"server { listen 8080; location / { autoindex ; } }");
}

static void test_autoindex_uppercase()
{
	expectException("autoindex_uppercase",
		"server { listen 8080; location / { autoindex ON; } }");
}

static void test_error_page_low_code()
{
	expectException("error_page_low_code",
		"server { listen 8080; location / { error_page 399 /err.html; } }");
}

static void test_error_page_non_numeric()
{
	expectException("error_page_non_numeric",
		"server { listen 8080; location / { error_page abc /err.html; } }");
}

static void test_error_page_200()
{
	expectException("error_page_200",
		"server { listen 8080; location / { error_page 200 /ok.html; } }");
}

static void test_error_page_300()
{
	expectException("error_page_300",
		"server { listen 8080; location / { error_page 300 /redirect.html; } }");
}

static void test_error_page_no_path()
{
	expectException("error_page_no_path",
		"server { listen 8080; location / { error_page 404 ; } }");
}

static void test_error_page_no_code()
{
	expectException("error_page_no_code",
		"server { listen 8080; location / { error_page ; } }");
}

static void test_error_page_float_code()
{
	expectException("error_page_float_code",
		"server { listen 8080; location / { error_page 404.5 /err.html; } }");
}

static void test_return_low_status()
{
	expectException("return_low_status",
		"server { listen 8080; location / { return 299 http://a.com; } }");
}

static void test_return_high_status()
{
	expectException("return_high_status",
		"server { listen 8080; location / { return 400 http://a.com; } }");
}

static void test_return_non_numeric()
{
	expectException("return_non_numeric",
		"server { listen 8080; location / { return abc http://a.com; } }");
}

static void test_return_200()
{
	expectException("return_200",
		"server { listen 8080; location / { return 200 http://a.com; } }");
}

static void test_return_500()
{
	expectException("return_500",
		"server { listen 8080; location / { return 500 http://a.com; } }");
}

static void test_return_no_url()
{
	expectException("return_no_url",
		"server { listen 8080; location / { return 301 ; } }");
}

static void test_return_no_status()
{
	expectException("return_no_status",
		"server { listen 8080; location / { return ; } }");
}

static void test_return_float_status()
{
	expectException("return_float_status",
		"server { listen 8080; location / { return 301.5 http://a.com; } }");
}

static void test_location_no_slash()
{
	expectException("location_no_slash",
		"server { listen 8080; location path { } }");
}

static void test_location_empty_path()
{
	expectException("location_empty_path",
		"server { listen 8080; location { } }");
}

static void test_index_no_value()
{
	expectException("index_no_value",
		"server { listen 8080; location / { index ; } }");
}

static void test_upload_store_no_value()
{
	expectException("upload_store_no_value",
		"server { listen 8080; location / { upload_store ; } }");
}

// ============== UNKNOWN DIRECTIVE TESTS ==============

static void test_unknown_server_directive()
{
	expectException("unknown_server_directive",
		"server { listen 8080; worker_processes 4; }");
}

static void test_unknown_location_directive()
{
	expectException("unknown_location_directive",
		"server { listen 8080; location / { proxy_pass http://backend; } }");
}

static void test_unknown_directive_include()
{
	expectException("unknown_directive_include",
		"server { listen 8080; include /etc/nginx/conf.d/*.conf; }");
}

static void test_unknown_directive_ssl()
{
	expectException("unknown_directive_ssl",
		"server { listen 8080; ssl on; }");
}

static void test_unknown_directive_access_log()
{
	expectException("unknown_directive_access_log",
		"server { listen 8080; access_log /var/log/access.log; }");
}

static void test_unknown_directive_error_log()
{
	expectException("unknown_directive_error_log",
		"server { listen 8080; error_log /var/log/error.log; }");
}

static void test_unknown_location_directive_fastcgi()
{
	expectException("unknown_location_directive_fastcgi",
		"server { listen 8080; location / { fastcgi_pass 127.0.0.1:9000; } }");
}

static void test_unknown_location_directive_alias()
{
	expectException("unknown_location_directive_alias",
		"server { listen 8080; location / { alias /var/www; } }");
}

static void test_unknown_location_directive_rewrite()
{
	expectException("unknown_location_directive_rewrite",
		"server { listen 8080; location / { rewrite ^/old /new permanent; } }");
}

static void test_unknown_location_directive_try_files()
{
	expectException("unknown_location_directive_try_files",
		"server { listen 8080; location / { try_files $uri $uri/ =404; } }");
}

// ============== MAIN ==============

int main()
{
	std::cout << "=== Config Parsing Tests ===" << std::endl << std::endl;

	std::cout << "--- Valid Configs ---" << std::endl;
	test_minimal_config();
	test_full_config();
	test_multiple_servers();
	test_multiple_locations();
	test_listen_port_only();
	test_multiple_listen();
	test_all_methods();
	test_autoindex_off();
	test_error_page_range();
	test_return_range();
	test_single_method_get();
	test_single_method_post();
	test_single_method_delete();
	test_multiple_error_pages();
	test_nested_path();

	std::cout << std::endl << "--- Syntax Errors ---" << std::endl;
	test_missing_server_open_brace();
	test_missing_close_brace();
	test_missing_semicolon();
	test_empty_file();
	test_no_server_block();
	test_location_missing_open_brace();
	test_double_open_brace();
	test_only_braces();
	test_server_no_content();
	test_location_missing_close_brace();

	std::cout << std::endl << "--- Invalid Values ---" << std::endl;
	test_listen_no_value();
	test_server_name_no_value();
	test_body_size_non_numeric();
	test_body_size_negative();
	test_body_size_float();
	test_body_size_with_suffix();
	test_root_no_slash();
	test_root_empty();
	test_location_root_no_slash();
	test_unknown_method();
	test_unknown_method_patch();
	test_unknown_method_head();
	test_unknown_method_options();
	test_method_lowercase();
	test_method_mixed_case();
	test_limit_except_no_value();
	test_autoindex_invalid();
	test_autoindex_true();
	test_autoindex_1();
	test_autoindex_empty();
	test_autoindex_uppercase();
	test_error_page_low_code();
	test_error_page_non_numeric();
	test_error_page_200();
	test_error_page_300();
	test_error_page_no_path();
	test_error_page_no_code();
	test_error_page_float_code();
	test_return_low_status();
	test_return_high_status();
	test_return_non_numeric();
	test_return_200();
	test_return_500();
	test_return_no_url();
	test_return_no_status();
	test_return_float_status();
	test_location_no_slash();
	test_location_empty_path();
	test_index_no_value();
	test_upload_store_no_value();

	std::cout << std::endl << "--- Unknown Directives ---" << std::endl;
	test_unknown_server_directive();
	test_unknown_location_directive();
	test_unknown_directive_include();
	test_unknown_directive_ssl();
	test_unknown_directive_access_log();
	test_unknown_directive_error_log();
	test_unknown_location_directive_fastcgi();
	test_unknown_location_directive_alias();
	test_unknown_location_directive_rewrite();
	test_unknown_location_directive_try_files();

	std::cout << std::endl << "=== Summary ===" << std::endl;
	std::cout << "Passed: " << g_passed << std::endl;
	std::cout << "Failed: " << g_failed << std::endl;

	return (g_failed > 0 ? 1 : 0);
}

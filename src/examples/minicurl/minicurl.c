/* Windows x64 / C89. Minimal explicit DLL declarations, no Windows SDK. */
#include <stdio.h>
#include <string.h>
#include <stddef.h>

typedef unsigned long DWORD;
typedef unsigned short WCHAR;
typedef void *HANDLE;

typedef struct {
    DWORD size;
    WCHAR *scheme;
    DWORD scheme_length;
    int scheme_id;
    WCHAR *host;
    DWORD host_length;
    unsigned short port;
    WCHAR *user;
    DWORD user_length;
    WCHAR *password;
    DWORD password_length;
    WCHAR *path;
    DWORD path_length;
    WCHAR *extra;
    DWORD extra_length;
} URL_PARTS;

int WinHttpCrackUrl(const WCHAR *, DWORD, DWORD, URL_PARTS *);
HANDLE WinHttpOpen(const WCHAR *, DWORD, const WCHAR *, const WCHAR *, DWORD);
int WinHttpSetTimeouts(HANDLE, int, int, int, int);
HANDLE WinHttpConnect(HANDLE, const WCHAR *, unsigned short, DWORD);
HANDLE WinHttpOpenRequest(HANDLE, const WCHAR *, const WCHAR *, const WCHAR *, const WCHAR *,
                          const WCHAR **, DWORD);
int WinHttpSendRequest(HANDLE, const WCHAR *, DWORD, void *, DWORD, DWORD, size_t);
int WinHttpReceiveResponse(HANDLE, void *);
int WinHttpQueryHeaders(HANDLE, DWORD, const WCHAR *, void *, DWORD *, DWORD *);
int WinHttpReadData(HANDLE, void *, DWORD, DWORD *);
int WinHttpCloseHandle(HANDLE);
DWORD GetLastError(void);
HANDLE GetStdHandle(DWORD);
int WriteFile(HANDLE, const void *, DWORD, DWORD *, void *);

static WCHAR url[8192], host[8192], path[8192], proxy[2048];
static unsigned char buffer[16384];
static const WCHAR agent[] = {'m', 'i', 'n', 'i', 'c', 'u', 'r', 'l', '/', '1', '.', '0', 0};
static const WCHAR method[] = {'G', 'E', 'T', 0};

static void usage(void) {
    fprintf(stderr, "Usage: minicurl [-o FILE] [--proxy HOST:PORT] URL\n"
                    "GET HTTP/HTTPS; follow redirects; write response body to stdout or FILE.\n"
                    "Use ASCII URLs with percent-encoded UTF-8 paths and punycode hostnames.\n"
                    "Exit: 0 success, 2 usage/URL, 3 network/TLS, 4 output, 22 HTTP >= 400.\n");
}

int main(int argc, char **argv) {
    const char *address, *filename, *operation, *proxy_name;
    HANDLE session, connection, request, output;
    FILE *file;
    URL_PARTS parts;
    DWORD status, length, received, written, offset, error;
    size_t i, j, n;
    int result;
    address = NULL;
    filename = NULL;
    file = NULL;
    proxy_name = NULL;
    session = NULL;
    connection = NULL;
    request = NULL;
    operation = "request";
    result = 3;
    for (i = 1; i < (size_t)argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            usage();
            return 0;
        }
        if (!strcmp(argv[i], "-o")) {
            if (filename || ++i >= (size_t)argc) {
                usage();
                return 2;
            }
            filename = argv[i];
        } else if (!strcmp(argv[i], "--proxy")) {
            if (proxy_name || ++i >= (size_t)argc) {
                usage();
                return 2;
            }
            proxy_name = argv[i];
        } else if (argv[i][0] == '-' || address) {
            usage();
            return 2;
        } else
            address = argv[i];
    }
    if (!address) {
        usage();
        return 2;
    }
    if (proxy_name) {
        n = strlen(proxy_name);
        if (!n || n >= 2048) {
            usage();
            return 2;
        }
        for (i = 0; i < n; i++) {
            if ((unsigned char)proxy_name[i] <= 32 || (unsigned char)proxy_name[i] >= 127 ||
                proxy_name[i] == '/' || proxy_name[i] == '@') {
                fprintf(stderr, "Proxy must be HOST:PORT without scheme or credentials\n");
                return 2;
            }
            proxy[i] = (unsigned char)proxy_name[i];
        }
        proxy[n] = 0;
    }
    /* Check the Windows LLP64 ABI used by the DLL declarations. */
    if (sizeof(DWORD) != 4 || sizeof(WCHAR) != 2 || sizeof(void *) != 8 ||
        sizeof(URL_PARTS) != 104 || offsetof(URL_PARTS, path) != 72) {
        fprintf(stderr, "Unsupported ABI\n");
        return 2;
    }
    n = strlen(address);
    if (n == 0 || n > 8189) {
        fprintf(stderr, "URL too long or empty\n");
        return 2;
    }
    for (i = 0; i < n && address[i] != '#'; i++) {
        if ((unsigned char)address[i] <= 32 || (unsigned char)address[i] >= 127) {
            fprintf(stderr, "URL must be ASCII without spaces/control characters\n");
            return 2;
        }
        url[i] = (unsigned char)address[i];
    }
    url[i] = 0;
    /* Supply '/' for host-only URLs with a query, which CrackUrl requires. */
    for (j = 0; j + 2 < i; j++) {
        if (url[j] == ':' && url[j + 1] == '/' && url[j + 2] == '/')
            break;
    }
    if (j + 2 >= i) {
        fprintf(stderr, "Expected http:// or https:// URL\n");
        return 2;
    }
    for (j += 3; j < i && url[j] != '/' && url[j] != '?'; j++) {
    }
    if (j < i && url[j] == '?') {
        for (n = i + 1; n > j; n--)
            url[n] = url[n - 1];
        url[j] = '/';
    }
    memset(&parts, 0, sizeof(parts));
    parts.size = sizeof(parts);
    parts.host = host;
    parts.host_length = 8192;
    parts.path = path;
    parts.path_length = 8192;
    parts.user_length = (DWORD)-1;
    parts.password_length = (DWORD)-1;
    if (!WinHttpCrackUrl(url, 0, 0, &parts) || (parts.scheme_id != 1 && parts.scheme_id != 2) ||
        !parts.host_length || parts.user_length || parts.password_length) {
        fprintf(stderr, "Invalid HTTP/HTTPS URL (embedded credentials unsupported)\n");
        return 2;
    }
    if (!path[0]) {
        path[0] = '/';
        path[1] = 0;
    }
    operation = "WinHttpOpen";
    /* AUTOMATIC_PROXY (4): use Windows proxy configuration. */
    session = WinHttpOpen(agent, proxy_name ? 3 : 4, proxy_name ? proxy : NULL, NULL, 0);
    if (!session)
        goto network_error;
    operation = "WinHttpSetTimeouts";
    if (!WinHttpSetTimeouts(session, 15000, 15000, 15000, 15000))
        goto network_error;
    operation = "WinHttpConnect";
    connection = WinHttpConnect(session, host, parts.port, 0);
    if (!connection)
        goto network_error;
    operation = "WinHttpOpenRequest";
    request = WinHttpOpenRequest(connection, method, path, NULL, NULL, NULL,
                                 parts.scheme_id == 2 ? 0x00800000UL : 0);
    if (!request)
        goto network_error;
    operation = "WinHttpSendRequest";
    if (!WinHttpSendRequest(request, NULL, 0, NULL, 0, 0, 0))
        goto network_error;
    operation = "WinHttpReceiveResponse";
    if (!WinHttpReceiveResponse(request, NULL))
        goto network_error;
    operation = "WinHttpQueryHeaders";
    length = sizeof(status);
    if (!WinHttpQueryHeaders(request, 19UL | 0x20000000UL, NULL, &status, &length, NULL))
        goto network_error;
    fprintf(stderr, "HTTP %lu\n", status);
    output = GetStdHandle((DWORD)-11);
    if (filename) {
        file = fopen(filename, "wb");
        if (!file) {
            fprintf(stderr, "Cannot open output file: %s\n", filename);
            result = 4;
            goto done;
        }
    }
    for (;;) {
        operation = "WinHttpReadData";
        if (!WinHttpReadData(request, buffer, sizeof(buffer), &received))
            goto network_error;
        if (!received)
            break;
        if (file) {
            if (fwrite(buffer, 1, received, file) != received)
                goto output_error;
        } else {
            offset = 0;
            while (offset < received) {
                if (!WriteFile(output, buffer + offset, received - offset, &written, NULL) ||
                    !written)
                    goto output_error;
                offset += written;
            }
        }
    }
    result = status >= 400 ? 22 : 0;
    goto done;
network_error:
    error = GetLastError();
    fprintf(stderr, "%s failed: Windows error %lu\n", operation, error);
    result = 3;
    goto done;
output_error:
    fprintf(stderr, "Output write failed\n");
    result = 4;
done:
    if (file && fclose(file) != 0) {
        fprintf(stderr, "Output close failed\n");
        result = 4;
    }
    if (request)
        WinHttpCloseHandle(request);
    if (connection)
        WinHttpCloseHandle(connection);
    if (session)
        WinHttpCloseHandle(session);
    return result;
}

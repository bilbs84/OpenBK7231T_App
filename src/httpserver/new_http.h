

extern const char httpHeader[];  // HTTP header
extern const char httpMimeTypeHTML[];              // HTML MIME type
extern const char httpMimeTypeText[];           // TEXT MIME type
extern const char htmlHeader[];
extern const char htmlEnd[];
extern const char htmlReturnToMenu[];

int HTTP_ProcessPacket(const char *recvbuf, char *outbuf, int outBufSize);


// callback function for http
typedef int (*http_callback_fn)(const char *payload, char *outbuf, int outBufSize);
// url MUST start with '/'
// urls must be unique (i.e. you can't have /about and /aboutme or /about/me)
int HTTP_RegisterCallback( const char *url, http_callback_fn callback);
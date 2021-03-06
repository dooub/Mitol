//
// Created by helidium on 3/24/17.
//
#ifndef MNS_REQUEST_H
#define MNS_REQUEST_H


#include <map>
#include <set>
#include <cstring>
#include <uv.h>
#include "Variables.h"
#include "Socket.h"

namespace MNS {
	// Forward declaration
	class SocketData;

	/**
	 * Request class. Implements the methods for request manipulation.
	 */
    class Request {
    public:
	    /**
	     * State of the request<br>
	     * <br>
	     * CONNECTING - The peer connected, but no data yet received...<br>
	     * READING_SOCKET - The peer sent data, reading...<br>
	     * PARSING_HEADERS - All the required data received, parsing the headers...<br>
	     * PARSING_BODY - All the headers parsed, parsing the body of the request...<br>
	     * FINISHED - Request is read and parsed, ready to be responded to...
	     */
        enum REQUEST_STATE {
            CONNECTING,
            READING_SOCKET,
            PARSING_HEADERS,
            PARSING_BODY,
            FINISHED,
	        NEED_MORE_DATA
        };

	    /**
	     * Default constructor
	     * @param socketData - SocketData for the request
	     */
        Request(const MNS::SocketData* socketData);

	    /// Default destructor
        ~Request();

	    /**
		 * Clears the request to be reused in case of persistent connections
		 * @return 0 on success, non zero on error
		 */
	    int clear();

	    /**
	     * Returns the finish state of the request
	     * @return true if request finished; false otherwise
	     */
	    bool isFinished();
	    /**
	     * Returns the request buffer
	     * @return Buffer containing the request data
	     */
        char* getBuffer();

	    /**
	     * Returns the body of the request
	     * @return Buffer containing the body of the request
	     */
	    char* getBodyBuffer();

	    /**
	     * Returns the size of the request buffer
	     * @return Size of the buffer
	     */
	    ssize_t getBufferSize();

	    /**
	     * Resizes the request buffer in case the buffer is smaller than the data
	     * @param newSize New size of the buffer
	     */
	    void resizeBuffer(int newSize);

	    /**
	     * Returns the lenght of the request data
	     * @return Length of the read request
	     */
	    ssize_t getBufferLen();

	    /**
	     * Returns the lenght of the request body data
	     * @return Length of the read request body
	     */
	    ssize_t getBodyBufferLen();

	    /**
	     * Parse the request
	     * @param requestLen - Length of the request to parse
	     * @return 0 on success, non zero on error
	     */
        int Parse(ssize_t requestLen);

	    /**
	     * Checks if the request contains header
	     * @param name - Name of the header to check
	     * @return TRUE if request contains the header, FALSE otherwise
	     */
        bool hasHeader(const std::string &name);

	    /**
	     * Returns the header value
	     * @param name - Name of the header
	     * @return Value of the header. NULL if not found
	     */
        std::string getHeader(const std::string &name);

	    /// map of the read headers
        std::map<std::string, std::string> headers;

	    /// Parsed HTTP_VERSION (1.1, 1.0)
        MNS::HTTP_VERSION httpVersion;

	    /// Parsed HTTP_METHOD (GET, POST, PUT, ...)
        MNS::HTTP_METHOD method;

	    /// Fd of the socket
        int socket;

	    /// Parsed URL (/...)
        const char *url;

	    /// State of the request
        REQUEST_STATE state;

    private:
	    /// Internal buffers
        char* buffer;
	    char* bodyBuffer;

	    /// Mark the finish of the request processing
	    bool finished;

	    ssize_t lastParsePos;
	    /// Internal buffer length
	    ssize_t bufferLen;

	    /// Internal body buffer length
	    ssize_t bodyBufferLen;

	    ssize_t bufferSize;

	    /// SocketData
        const MNS::SocketData *socketData;

    protected:
    };
}

#endif //MNS_REQUEST_H

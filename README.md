# 42_webserv

### HTTP version
1.0 (RFC 1945) + cookies from 1.1

### HTTP ABNF grammar for each request

the following grammar is slightly adapted from HTTP 1.0 depending on our use case (i.e. no absoluteURI in Request-URI because we don't have proxies, no request-header, etc)

###### Request
Request       	= Simple-Request | Full-Request
Simple-Request	= "GET" SP Request-URI CRLF
Full-Request	= Request-Line *(Content-Encoding | Content-Length | Content-Type | Cookie) ; my own modified version
					CRLF
					[ Entity-Body ]

###### Request-Line
Request-Line	= Method SP Request-URI SP HTTP-Version CRLF
Method			= "GET" | "POST" | "DELETE"
Request-URI		= abs_path
HTTP-Version	= "HTTP" "/" 1*DIGIT "." 1*DIGIT

abs_path       	= "/" rel_path
rel_path       	= [ path ] [ ";" params ] [ "?" query ]
path           	= fsegment *( "/" segment )
fsegment       	= 1*pchar
segment        	= *pchar
params         	= param *( ";" param )
param          	= *( pchar | "/" )
query          	= *( uchar | reserved )
pchar          	= uchar | ":" | "@" | "&" | "=" | "+"
uchar          	= unreserved | escape
unreserved     	= ALPHA | DIGIT | safe | extra | national
escape         	= "%" HEX HEX
reserved       	= ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+"
extra          	= "!" | "*" | "'" | "(" | ")" | ","
safe           	= "$" | "-" | "_" | "."
unsafe         	= CTL | SP | <"> | "#" | "%" | "<" | ">"
national       	= <any OCTET excluding ALPHA, DIGIT,reserved, extra, safe, and unsafe>

EVERY HEADER IS CRLF TERMINATED
###### General-Header
Date           	= "Date" ":" HTTP-date
HTTP-Date		= <whole bunch of weird stuff, do not implement unless absolutely required>

###### Entity-Header
Content-Encoding 	= "Content-Encoding" ":" content-coding
content-coding 		= "x-gzip" | "x-compress" | token

Content-Length 		= "Content-Length" ":" 1*DIGIT ;

Content-Type   		= "Content-Type" ":" media-type
media-type     		= type "/" subtype *( ";" parameter )
type           		= token
subtype        		= token
parameter      		= attribute "=" value
attribute      		= token
value          		= token | quoted-string

###### ABNF Rules

OCTET          = <any 8-bit sequence of data>
CHAR           = <any US-ASCII character (octets 0 - 127)>
UPALPHA        = <any US-ASCII uppercase letter "A".."Z">
LOALPHA        = <any US-ASCII lowercase letter "a".."z">
ALPHA          = UPALPHA | LOALPHA
DIGIT          = <any US-ASCII digit "0".."9">
CTL            = <any US-ASCII control character (octets 0 - 31) and DEL (127)>
CR             = <US-ASCII CR, carriage return (13)>
LF             = <US-ASCII LF, linefeed (10)>
SP             = <US-ASCII SP, space (32)>
HT             = <US-ASCII HT, horizontal-tab (9)>

LWS            = [CRLF] 1*( SP | HT )
CRLF           = CR LF
TEXT           = <any OCTET except CTLs, but including LWS>
word           = token | quoted-string
token          = 1*<any CHAR except CTLs or tspecials>
tspecials      = "(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\" | <">
				| "/" | "[" | "]" | "?" | "=" | "{" | "}" | SP | HT

quoted-string  = ( <"> *(qdtext) <"> )

qdtext         = <any CHAR except <"> and CTLs, but including LWS>

###### Grammar rules

name = definition
	self explanatory
"literal"
	quotes surrond litteral text
rule1 | rule2
	alternatives (OR)
(rule1 rule2)
	elems between par. are treated as single elems, i.e. (elem (foo | bar) elem) allows "elem foo elem" or "elem bar elem"
*rule
	indicates repetion and is of form <n>*<m>elem. Default are 0 and inf. *elem --> any number. 1*elem --> min 1 num. *3elem --> at most 3 nums. 1*3elem --> between 1 and 3 elems
[rule]
	optional elements
N rule
	<n>*<n>elem
#rule
	like *elem but for lists that are comma separated
implied *LWS
	implied LWS between two adjacent **words** 


#### Stress test

- create network: docker network create test-network
- build tester container: docker build -t test-env -f Dockerfile.tester . *in the stress-test directory*
- build server container: docker build -t serv-env -f Dockerfile.server .
- start server: docker run -it --rm --network=test-network -p 9090:9090 -p 8080:8080 --name=server -v /home/mturgeon/rank5/webserv:/webserv serv-env bash
- start tester: docker run -it --rm --network=test-network --name=tester -v /home/mturgeon/rank5/webserv/tests/stress_test/:/data test-env bash
- run the tests as bash commands in the containers
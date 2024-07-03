SRCS=xml.c xml_parser.c xml_lexer.c
OBJS=$(SRCS:.c=.o)

TARGET=xml_parser

DEBUG_FLAG=-g
DEFINE_FLAG=-DDEBUG
#LEX_DEBUG=-DLEX_DEBUG
CFLAGS=${DEBUG_FLAG} ${DEFINE_FLAG} ${LEX_DEBUG} -I.
LDFLAGS=

all:${TARGET}

${TARGET}:${OBJS}
	${CC} -o $@ ${OBJS} ${LDFALGS}

clean:
	-rm -f ${OBJS} ${TARGET}

.c.o:
	${CC} ${CFLAGS} -c $< -o $@


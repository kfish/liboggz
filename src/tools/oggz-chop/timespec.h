#ifndef __TIMESPEC_H__
#define __TIMESPEC_H__

double parse_timespec (const char * str);

/**
 * Parse the name=value pairs in the query string and set
 * time start and end parameters.
 * @param state The OCState to store the time range into
 * @param query The query string
 */
void parse_query (OCState * state, char * query);

#endif /* __TIMESPEC_H__ */

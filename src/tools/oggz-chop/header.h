#ifndef __HEADER_H__
#define __HEADER_H__

int header_status_206 (void);
int header_status_416 (void);
int header_accept_timeuri_ogg (void);
int header_content_type_ogg (void);
int header_content_length (oggz_off_t len);
int header_content_duration (double duration);
int header_content_disposition_attachment (char * filename);
int header_content_range_bytes (oggz_off_t range_start,
                                oggz_off_t range_end,
                                oggz_off_t size);
int header_content_range_star (oggz_off_t size);
int header_accept_ranges (void);
int header_last_modified (time_t mtime);
int header_not_modified (void);
int header_end (void);

#endif /* __HEADER_H__ */

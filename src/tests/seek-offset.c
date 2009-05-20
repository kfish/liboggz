/*
 * Annodex Association 2009
*/

/** Generate a pathological seek file */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include "oggz/oggz.h"

#include "oggz_tests.h"

/* #define DEBUG */

#define DATA_BUF_LEN (4096*32)
#define MAX_PACKET 25

static char * whence_words[3] = {"SEEK_SET", "SEEK_CUR", "SEEK_END"};

static long serialno;
static int read_called = 0;

static int offset_end = 0;
static int my_offset = 0;

/* Stored read positions */
static oggz_position positions[MAX_PACKET];

/* Expected read state */
static int read_iter = 0;
static long read_b_o_s = 1;
static long read_e_o_s = 0;

#define CHARCODE(x) ('a' + x)

static int
hungry (OGGZ * oggz, int empty, void * user_data)
{
  unsigned char buf[8192];
  ogg_packet op;
  static int iter = 0;
  static long b_o_s = 1;
  static long e_o_s = 0;
  int flush = 0;
  int packet_len;
  long ret;

  if (iter >= MAX_PACKET) return 1;

  /* Mix it up a bit, with a few of my favourite primes */
  if (iter > 13 && iter < 19)
    packet_len = iter;
  else if (iter % 3 == 0)
    packet_len = 937;
  else if (iter % 11 == 0) 
    packet_len = 3947;
  else
    packet_len = 5557;

  memset (buf, CHARCODE(iter), packet_len);

  op.packet = buf;
  op.bytes = packet_len;
  op.b_o_s = b_o_s;
  op.e_o_s = e_o_s;
  op.granulepos = iter;
  op.packetno = iter;

  /* Main check */
  if ((ret = oggz_write_feed (oggz, &op, serialno, flush, NULL)) != 0) {
    printf ("oggz_write_feed returned %ld\n", ret);
    FAIL ("Oggz write failed");
  }

  iter++;
  b_o_s = 0;
  if (iter == MAX_PACKET-1) e_o_s = 1;
  
  return 0;
}

static int
read_packet_stash (OGGZ * oggz, oggz_packet * zp, long serialno, void * user_data)
{
  ogg_packet * op = &zp->op;
#ifdef DEBUG
  printf ("%08" PRI_OGGZ_OFF_T "x: serialno %010lu, "
	  "granulepos %" PRId64 ", packetno %" PRId64,
	  oggz_tell (oggz), serialno, op->granulepos, op->packetno);

  if (op->b_o_s) {
    printf (" *** bos");
  }

  if (op->e_o_s) {
    printf (" *** eos");
  }

  printf ("\n");
#endif

  if (op->packet[0] != CHARCODE(read_iter))
    FAIL ("Packet contains incorrect data");

  if ((op->b_o_s == 0) != (read_b_o_s == 0))
    FAIL ("Packet has incorrect b_o_s");

  if ((op->e_o_s == 0) != (read_e_o_s == 0))
    FAIL ("Packet has incorrect e_o_s");

  if (op->granulepos != -1 && op->granulepos != read_iter)
    FAIL ("Packet has incorrect granulepos");

  if (op->packetno != read_iter)
    FAIL ("Packet has incorrect packetno");

  /* Stash the position */
  memcpy (&positions[read_iter], &zp->pos, sizeof (oggz_position));

  read_iter++;
  read_b_o_s = 0;
  if (read_iter == MAX_PACKET-1) {
    read_e_o_s = 1;
  }

  return 0;
}

static int
read_packet_test (OGGZ * oggz, oggz_packet * zp, long serialno, void * user_data)
{
  ogg_packet * op = &zp->op;
  oggz_position * expect_pos;
  char buf[256];

#ifdef DEBUG
  printf ("0x%08" PRI_OGGZ_OFF_T "x: serialno %010lu, "
	  "granulepos %" PRId64 ", packetno %" PRId64,
	  oggz_tell (oggz), serialno, op->granulepos, op->packetno);

  if (op->b_o_s) {
    printf (" *** bos");
  }

  if (op->e_o_s) {
    printf (" *** eos");
  }

  printf ("\n");
#endif

  if (op->packet[0] != CHARCODE(read_iter)) {
    snprintf (buf, 256, "Packet contains incorrect data %c, expected %c\n",
              op->packet[0], CHARCODE(read_iter));
    FAIL (buf);
#ifdef DEBUG
  } else {
    printf ("Packet contains correct data %c\n", op->packet[0]);
#endif
  }

  if (op->granulepos != -1 && op->granulepos != read_iter)
    FAIL ("Packet has incorrect granulepos");

  expect_pos = &positions[read_iter];

#ifdef DEBUG
  printf ("  Expecting: begin_page 0x%llx\tend_page 0x%llx\tpages %d\tsegment %d\n",
          expect_pos->begin_page_offset, expect_pos->end_page_offset,
          expect_pos->pages, expect_pos->begin_segment_index);
#endif

#if 0
  /* oggz_tell() returns the start of the current page, but the current packet may
   * start on an earlier page.
   */
  if (oggz_tell (oggz) != expect_pos->begin_page_offset) {
    snprintf (buf, 256, "Reader has incorrect offset 0x%llx, expected 0x%llx",
              oggz_tell(oggz), expect_pos->begin_page_offset);
    FAIL (buf);
  }
#endif

  if (zp->pos.begin_page_offset != expect_pos->begin_page_offset) {
    snprintf (buf, 256, "Returned position has incorrect begin_page_offset 0x%llx, expected 0x%llx",
              zp->pos.begin_page_offset, expect_pos->begin_page_offset);
    FAIL (buf);
  }

  if (zp->pos.end_page_offset != expect_pos->end_page_offset) {
    snprintf (buf, 256, "Returned position has incorrect end_page_offset 0x%llx, expected 0x%llx",
              zp->pos.end_page_offset, expect_pos->end_page_offset);
    FAIL (buf);
  }

  if (zp->pos.pages != expect_pos->pages) {
    snprintf (buf, 256, "Returned position has incorrect pages %d, expected %d",
              zp->pos.pages, expect_pos->pages);
    FAIL (buf);
  }

  if (zp->pos.begin_segment_index != expect_pos->begin_segment_index) {
    snprintf (buf, 256, "Returned position has incorrect begin_segment_index %d, expected %d",
              zp->pos.begin_segment_index, expect_pos->begin_segment_index);
    FAIL (buf);
  }

  read_iter++;

  /* Got correct seek position, no need to check later packets */
  return OGGZ_STOP_OK;
}
static size_t
my_io_read (void * user_handle, void * buf, size_t n)
{
  unsigned char * data_buf = (unsigned char *)user_handle;
  int len;

  /* Mark that the read IO method was actually used */
  read_called++;

  len = MIN ((int)n, offset_end - my_offset);
  memcpy (buf, &data_buf[my_offset], len);

  my_offset += len;

  return len;
}

static int
my_io_seek (void * user_handle, long offset, int whence)
{
  switch (whence) {
  case SEEK_SET:
    my_offset = offset;
    break;
  case SEEK_CUR:
    my_offset += offset;
    break;
  case SEEK_END:
    my_offset = offset_end + offset;
    break;
  default:
    return -1;
  }

  return 0;
}

static long
my_io_tell (void * user_handle)
{
  return my_offset;
}

int
read_setup (OGGZ * reader, long n)
{
  INFO ("Caching packet positions");

  oggz_set_read_callback (reader, -1, read_packet_stash, NULL);

  oggz_read (reader, n);

  if (read_called == 0)
    FAIL("Read method ignored");

  return 0;
}

off_t
test_seek_to_offset (OGGZ * reader, long n, off_t offset, int whence, off_t correct, int i)
{
  char buf[64];
  off_t result, nread;
  oggz_position * pos;

  snprintf (buf, 64, "+ Seeking to offset 0x%08llx %s", offset, whence_words[whence]);
  INFO (buf);

  read_iter = i;
  read_b_o_s = (i==0);
  read_e_o_s = (i==MAX_PACKET-1);

  result = oggz_seek (reader, offset, whence);

  read_called = 0;

#ifdef DEBUG
  printf ("= Result: 0x%08llx\n", result);
#endif

  if (result != correct)
    FAIL ("oggz_seek() returned incorrect offset");

  while ((nread = oggz_read (reader, n-result)) > 0) {
    result += nread;
#ifdef DEBUG
    printf ("%s: Read %ld bytes\n", __func__, nread);
#endif
  }

  if (read_called == 0)
    FAIL("Read method ignored after seeking");

#ifdef DEBUG
  putchar ('\n');
#endif

  return 0;
}

int
seek_test (OGGZ * reader, long n)
{
  off_t off;
  int i;

  oggz_set_read_callback (reader, -1, read_packet_test, NULL);

  test_seek_to_offset (reader, n, 0x10000, SEEK_SET, 0x11097, 23);
  test_seek_to_offset (reader, n, 0x05000, SEEK_SET, 0x0585d, 6);
  test_seek_to_offset (reader, n, 0x0a000, SEEK_SET, 0x0ac6a, 12);
  test_seek_to_offset (reader, n, 0x02000, SEEK_SET, 0x025c9, 3);
  test_seek_to_offset (reader, n, 0x0f000, SEEK_SET, 0x0ffcf, 22);
  test_seek_to_offset (reader, n, 0x01000, SEEK_END, 0x1212d, 24);
  test_seek_to_offset (reader, n, 0x10000, SEEK_END, 0x0365c, 5);

#if 0
  oggz_seek (reader, 0x01000, SEEK_SET);
  test_seek_to_offset (reader, n, 0x07000, SEEK_CUR, 0x08af1, 9);

  oggz_seek (reader, 0x10000, SEEK_SET);
  test_seek_to_offset (reader, n, -0x03000, SEEK_CUR, 0x0de03, 20);

  oggz_seek (reader, 0x10000, SEEK_SET);
  test_seek_to_offset (reader, n, 0x02000, SEEK_CUR, 0x1212d, 24);
#endif

  return 0;
}

int
main (int argc, char * argv[])
{
  OGGZ * reader, * writer;
  unsigned char data_buf[DATA_BUF_LEN];
  long n;

  INFO ("Testing oggz_seek()");

  writer = oggz_new (OGGZ_WRITE);
  if (writer == NULL)
    FAIL("newly created OGGZ writer == NULL");

  serialno = oggz_serialno_new (writer);

  if (oggz_write_set_hungry_callback (writer, hungry, 1, NULL) == -1)
    FAIL("Could not set hungry callback");

  reader = oggz_new (OGGZ_READ);
  if (reader == NULL)
    FAIL("newly created OGGZ reader == NULL");

  oggz_io_set_read (reader, my_io_read, data_buf);
  oggz_io_set_seek (reader, my_io_seek, data_buf);
  oggz_io_set_tell (reader, my_io_tell, data_buf);

  INFO ("Generating Ogg data with pathological paging");
  n = oggz_write_output (writer, data_buf, DATA_BUF_LEN);

  if (n >= DATA_BUF_LEN)
    FAIL("Too much data generated by writer");

  offset_end = n;

#ifdef DEBUG
  printf ("Generated %ld bytes\n", n);
  {
    FILE * outfile;

#define OUTFILE "/tmp/seek-patho.ogg"
    INFO ("  Writing to " OUTFILE);
    outfile = fopen (OUTFILE, "w");
    fwrite (data_buf, 1, n, outfile);
    fclose (outfile);
  }
#endif

  read_setup (reader, n);

  seek_test (reader, n);

  if (oggz_close (reader) != 0)
    FAIL("Could not close OGGZ reader");

  if (oggz_close (writer) != 0)
    FAIL("Could not close OGGZ writer");

  exit (0);
}

/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __OGGZ_H__
#define __OGGZ_H__

#include <ogg/ogg.h>
#include <oggz/oggz_constants.h>

/** \mainpage
 *
 * \section intro Oggz makes programming with Ogg easy!
 *
 * This is the documentation for the Oggz C API. Oggz provides a simple
 * programming interface for reading and writing Ogg files and streams.
 * Ogg is an interleaving data container developed by Monty
 * at <a href="http://www.xiph.org/">Xiph.Org</a>, originally to
 * support the Ogg Vorbis audio format.
 *
 * liboggz supports the flexibility afforded by the Ogg file format
 *
 * - A simple, callback based open/read/close or open/write/close interface
 *   to all Ogg files
 * - A customisable seeking abstraction for seeking on multitrack Ogg data
 * 
 * \subsection contents Contents
 *
 * - \link basics Basics \endlink:
 * Information about Ogg required to understand liboggz
 *
 * - \link oggz.h oggz.h \endlink:
 * Documentation of the Oggz API
 *
 * - \link configuration Configuration \endlink:
 * Customizing liboggz to only read or write.
 *
 * - \link building Building \endlink:
 * Information related to building software that uses liboggz.
 *
 * \section Licensing
 *
 * liboggz is provided under the following BSD-style open source license:
 *
 * \include COPYING
 *
 */

/** \defgroup basics Ogg basics
 *
 * \section Scope
 *
 * This section provides a minimal introduction to Ogg concepts, covering
 * only that which is required to use liboggz.
 *
 * For more detailed information, see the
 * <a href="http://www.xiph.org/ogg/">Ogg</a> homepage
 * or IETF <a href="http://www.ietf.org/rfc/rfc3533.txt">RFC 3533</a>
 * <i>The Ogg File Format version 0</i>.
 *
 * \section Terminology
 *
 * \subsection Bitstreams
 *
 * Physical bitstreams contain interleaved logical bitstreams.
 * Each logical bitstream is uniquely identified by a serial number or
 * \a serialno.
 *
 * - \a serialno: an integer identifying a logical bitstream.
 *
 * \subsection Packets
 *
 * 
 *
 */

/** \defgroup configuration Configuration
 * \section ./configure ./configure
 *
 * It is possible to customize the functionality of liboggz
 * by using various ./configure flags when
 * building it from source. You can build a smaller
 * version of liboggz to only read or write.
 * By default, both reading and writing support is built.
 *
 * For general information about using ./configure, see the file
 * \link install INSTALL \endlink
 *
 * \subsection no_encode Removing writing support
 *
 * Configuring with \a --disable-write will remove all support for writing:
 * - All internal write related functions will not be built
 * - Any attempt to call oggz_new(), oggz_open() or oggz_openfd()
 *   with \a flags == OGGZ_WRITE will fail, returning NULL
 * - Any attempt to call oggz_write(), oggz_write_output(), oggz_write_feed(),
 *   oggz_write_set_hungry_callback(), or oggz_write_get_next_page_size()
 *   will return OGGZ_ERR_DISABLED
 *
 * \subsection no_decode Removing reading support
 *
 * Configuring with \a --disable-read will remove all support for reading:
 * - All internal reading related functions will not be built
 * - Any attempt to call oggz_new(), oggz_open() or oggz_openfd()
 *    with \a flags == OGGZ_READ will fail, returning NULL
 * - Any attempt to call oggz_read(), oggz_read_input(),
 *   oggz_set_read_callback(), oggz_seek(), or oggz_seek_units() will return 
 *   OGGZ_ERR_DISABLED
 *
 */

/** \defgroup install Installation
 * \section install INSTALL
 *
 * \include INSTALL
 */

/** \defgroup building Building against liboggz
 *
 *
 * \section autoconf Using GNU autoconf
 *
 * If you are using GNU autoconf, you do not need to call pkg-config
 * directly. Use the following macro to determine if liboggz is
 * available:
 *
 <pre>
 PKG_CHECK_MODULES(OGGZ, oggz >= 0.6.0,
                   HAVE_OGGZ="yes", HAVE_OGGZ="no")
 if test "x$HAVE_OGGZ" = "xyes" ; then
   AC_SUBST(OGGZ_CFLAGS)
   AC_SUBST(OGGZ_LIBS)
 fi
 </pre>
 *
 * If liboggz is found, HAVE_OGGZ will be set to "yes", and
 * the autoconf variables OGGZ_CFLAGS and OGGZ_LIBS will
 * be set appropriately.
 *
 * \section pkg-config Determining compiler options with pkg-config
 *
 * If you are not using GNU autoconf in your project, you can use the
 * pkg-config tool directly to determine the correct compiler options.
 *
 <pre>
 OGGZ_CFLAGS=`pkg-config --cflags oggz`

 OGGZ_LIBS=`pkg-config --libs oggz`
 </pre>
 *
 */

/** \file
 * The liboggz C API.
 * 
 * \section general Generic semantics
 *
 * All access is managed via an OGGZ handle. This can be instantiated
 * in one of three ways:
 *
 * - oggz_open() - Open a full pathname
 * - oggz_openfd() - Use an already opened file descriptor
 * - oggz_new() - Create an anonymous OGGZ object, which you can later
 *   handle via memory buffers
 *
 * To finish using an OGGZ handle, it should be closed with oggz_close().
 *
 * \section reading Reading Ogg data
 *
 * To read from Ogg files or streams you must instantiate an OGGZ handle
 * with flags set to OGGZ_READ, and provide an OggzReadPacket
 * callback with oggz_set_read_callback().
 * See the \ref read_api section for details.
 *
 * \section writing Writing Ogg data
 *
 * To write to Ogg files or streams you must instantiate an OGGZ handle
 * with flags set to OGGZ_WRITE, and provide an OggzWritePacket
 * callback with oggz_set_write_callback().
 * See the \ref write_api section for details.
 *
 * \section headers Headers
 *
 * oggz.h provides direct access to libogg types such as ogg_packet, defined
 * in <ogg/ogg.h>.
 */

/** \defgroup seeking_group Seeking
 * \section seeking Seeking
 *
 * The seeking semantics of the Ogg file format were outlined by Monty in
 * <a href="http://www.xiph.org/archives/theora-dev/200209/0040.html">a
 * post to theora-dev</a> in September 2002. Quoting from that post, we
 * have the following assumptions:
 *
 * - Ogg is not a non-linear format. ... It is a media transport format
 *   designed to do nothing more than deliver content, in a stream, and
 *   have all the pieces arrive on time and in sync.
 * - The Ogg layer does not know the specifics of the codec data it's
 *   multiplexing into a stream. It knows nothing beyond 'Oooo, packets!',
 *   that the packets belong to different buckets, that the packets go in
 *   order, and that packets have position markers. Ogg does not even have
 *   a concept of 'time'; it only knows about the sequentially increasing,
 *   unitless position markers. It is up to higher layers which have
 *   access to the codec APIs to assign and convert units of framing or
 *   time.
 *
 * liboggz provides two abstractions for seeking at an arbitrary level of
 * precision, as well as allowing seeking to a direct byte offset.
 *
 * To seek across non-metric spaces for which a partial order
 * exists (ie. data that is not synchronised by a measure such as time, but
 * is nevertheless somehow seekably structured), use an OggzOrder.
 *
 * For most purposes, such as media data, use an OggzMetric instead.
 */

/**
 * An opaque handle to an Ogg file. This is returned by oggz_open() or
 * oggz_new(), and is passed to all other oggz_* functions.
 */
typedef void * OGGZ;

/**
 * Create a new OGGZ object
 * \param flags OGGZ_READ or OGGZ_WRITE
 * \returns A new OGGZ object
 * \retval NULL on system error; check errno for details
 */
OGGZ * oggz_new (int flags);

/**
 * Open an Ogg file, creating an OGGZ handle for it
 * \param filename The file to open
 * \param flags OGGZ_READ or OGGZ_WRITE
 * \return A new OGGZ handle
 * \retval NULL System error; check errno for details
 */
OGGZ * oggz_open (char * filename, int flags);

/**
 * Create an OGGZ handle associated with a file descriptor.
 * \param fd An open file descriptor
 * \param flags OGGZ_READ or OGGZ_WRITE
 * \returns A new OGGZ handle
 * \retval NULL System error; check errno for details
 */
OGGZ * oggz_openfd (int fd, int flags);

/**
 * Ensure any associated file descriptors are flushed.
 * \param oggz An OGGZ handle
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 * \retval OGGZ_ERR_SYSTEM System error; check errno for details
 */
int oggz_flush (OGGZ * oggz);

/**
 * Close an OGGZ handle
 * \param oggz An OGGZ handle
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_SYSTEM System error; check errno for details
 */
int oggz_close (OGGZ * oggz);

/**
 * Determine if a given logical bitstream is at bos (beginning of stream).
 * \param oggz An OGGZ handle
 * \param serialno Identify a logical bitstream within \a oggz, or -1 to
 * query if all logical bitstreams in \a oggz are at bos
 * \retval 1 The given stream is at bos
 * \retval 0 The given stream is not at bos
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 */
int oggz_get_bos (OGGZ * oggz, long serialno);

/**
 * Determine if a given logical bitstream is at eos (end of stream).
 * \param oggz An OGGZ handle
 * \param serialno Identify a logical bitstream within \a oggz, or -1 to
 * query if all logical bitstreams in \a oggz are at eos
 * \retval 1 The given stream is at eos
 * \retval 0 The given stream is not at eos
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 */
int oggz_get_eos (OGGZ * oggz, long serialno);

/** \defgroup read_api OGGZ Read API
 *
 * \{
 */

/** 
 * This is the signature of a callback which you must provide for Oggz
 * to call whenever it finds a new packet in the Ogg stream associated
 * with \a oggz.
 *
 * \param oggz The OGGZ handle
 * \param op The full ogg_packet (see <ogg/ogg.h>)
 * \param serialno Identify the logical bistream in \a oggz that contains
 *                 \a op
 * \param user_data A generic pointer you have provided earlier
 * \returns 0 to continue, non-zero to instruct OGGZ to stop.
 *
 * \note It is possible to provide different callbacks per logical
 * bitstream -- see oggz_set_read_callback() for more information.
 */
typedef int (*OggzReadPacket) (OGGZ * oggz, ogg_packet * op, long serialno,
			       void * user_data);

/**
 * Set a callback for Oggz to call when a new Ogg packet is found in the
 * stream.
 *
 * \param oggz An OGGZ handle previously opened for reading
 * \param serialno Identify the logical bitstream in \a oggz to attach
 * this callback to, or -1 to attach this callback to all unattached
 * logical bitstreams in \a oggz.
 * \param read_packet Your callback function
 * \param user_data Arbitrary data you wish to pass to your callback
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 *
 * \note Values of \a serialno other than -1 allows you to specify different
 * callback functions for each logical bitstream.
 *
 * \note It is safe to call this callback from within an OggzReadPacket
 * function, in order to specify that subsequent packets should be handled
 * by a different OggzReadPacket function.
 */
int oggz_set_read_callback (OGGZ * oggz, long serialno,
			    OggzReadPacket read_packet, void * user_data);

/**
 * Read n bytes into \a oggz, calling any read callbacks on the fly.
 * \param oggz An OGGZ handle previously opened for reading
 * \param n A count of bytes to ingest
 * \retval ">  0" The number of bytes successfully ingested.
 * \retval 0 End of file
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 * \retval OGGZ_ERR_SYSTEM System error; check errno for details
 */
long oggz_read (OGGZ * oggz, long n);

/**
 * Input data into \a oggz.
 * \param oggz An OGGZ handle previously opened for reading
 * \param buf A memory buffer
 * \param n A count of bytes to input
 * \retval ">  0" The number of bytes successfully ingested.
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
long oggz_read_input (OGGZ * oggz, unsigned char * buf, long n);

/** \}
 */

/** \defgroup write_api OGGZ Write API
 *
 * \{
 */

/**
 * This is the signature of a callback which Oggz will call when \a oggz
 * is \link hungry hungry \endlink.
 *
 * \param oggz The OGGZ handle
 * \param empty A value of 1 indicates that the packet queue is currently
 *        empty. A value of 0 indicates that the packet queue is not empty.
 * \param user_data A generic pointer you have provided earlier
 * \retval 0 Continue
 * \retval non-zero Instruct OGGZ to stop.
 */
typedef int (*OggzWriteHungry) (OGGZ * oggz, int empty, void * user_data);

/**
 * Set a callback for Oggz to call when \a oggz
 * is \link hungry hungry \endlink.
 *
 * \param oggz An OGGZ handle previously opened for writing
 * \param hungry Your callback function
 * \param only_when_empty When to call: a value of 0 indicates that
 * OGGZ should call \a hungry() after each and every packet is written;
 * a value of 1 indicates that OGGZ should call \a hungry() only when
 * its packet queue is empty
 * \param user_data Arbitrary data you wish to pass to your callback
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 * \note Passing a value of 0 for \a only_when_empty allows you to feed
 * new packets into \a oggz's packet queue on the fly.
 */
int oggz_write_set_hungry_callback (OGGZ * oggz,
				    OggzWriteHungry hungry,
				    int only_when_empty,
				    void * user_data);
/**
 * Add a packet to \a oggz's packet queue.
 * \param oggz An OGGZ handle previously opened for writing
 * \param op An ogg_packet with all fields filled in
 * \param serialno Identify the logical bitstream in \a oggz to add the
 * packet to
 * \param flush Whether to flush this packet to the stream
 * \param guard A guard for nocopy, NULL otherwise
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_GUARD \a guard specified has non-zero initialization
 * \retval OGGZ_ERR_BOS Packet would be bos packet of a new logical bitstream,
 *         but oggz has already written one or more non-bos packets in
 *         other logical bitstreams,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_EOS The logical bitstream identified by \a serialno is
 *         already at eos,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_BYTES \a op->bytes is invalid,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_B_O_S \a op->b_o_s is invalid,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_GRANULEPOS \a op->granulepos is less than that of
 *         an earlier packet within this logical bitstream,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_PACKETNO \a op->packetno is less than that of an
 *         earlier packet within this logical bitstream,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 *         logical bitstream in \a oggz,
 *         and \a oggz is not flagged OGGZ_NONSTRICT
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 *
 * \note If \a op->b_o_s is initialized to \a -1 before calling
 *       oggz_write_feed(), Oggz will fill it in with the appropriate
 *       value; ie. 1 for the first packet of a new stream, and 0 otherwise.
 */
int oggz_write_feed (OGGZ * oggz, ogg_packet * op, long serialno, int flush,
		     int * guard);

/**
 * Output data from an OGGZ handle. Oggz will call your write callback
 * as needed.
 *
 * \param oggz An OGGZ handle previously opened for writing
 * \param buf A memory buffer
 * \param n A count of bytes to output
 * \retval "> 0" The number of bytes successfully output
 * \retval 0 End of stream
 * \retval OGGZ_ERR_RECURSIVE_WRITE Attempt to initiate writing from
 * within an OggzHungry callback
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
long oggz_write_output (OGGZ * oggz, unsigned char * buf, long n);

/**
 * Write n bytes from an OGGZ handle. Oggz will call your write callback
 * as needed.
 *
 * \param oggz An OGGZ handle previously opened for writing
 * \param n A count of bytes to be written
 * \retval "> 0" The number of bytes successfully output
 * \retval 0 End of stream
 * \retval OGGZ_ERR_RECURSIVE_WRITE Attempt to initiate writing from
 * within an OggzHungry callback
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
long oggz_write (OGGZ * oggz, long n);

/**
 * Query the number of bytes in the next page to be written.
 *
 * \param oggz An OGGZ handle previously opened for writing
 * \retval ">= 0" The number of bytes in the next page
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
long oggz_write_get_next_page_size (OGGZ * oggz);

/** \}
 */

/** \defgroup metric OggzMetric
 *
 * If every position in an Ogg stream can be described by a metric (eg. time)
 * then define this function that returns some arbitrary unit value.
 * This is the normal use of OGGZ for media streams. The meaning of units is
 * arbitrary, but must be consistent across all logical bitstreams; for
 * example a conversion of the time offset of a given packet into nanoseconds
 * or a similar stream-specific subdivision may be appropriate.
 *
 * To use OggzMetric:
 *
 * - Implement an OggzMetric callback
 * - Set the OggzMetric callback using oggz_set_metric()
 * - To seek, use oggz_seek_units(). Oggz will perform a ratio search
 *   through the Ogg bitstream, using the OggzMetric callback to determine
 *   its position relative to the desired unit.
 *
 * \note
 *
 * Many data streams begin with headers describing such things as codec
 * setup parameters. One of the assumptions Monty makes is:
 *
 * - Given pre-cached decode headers, a player may seek into a stream at
 *   any point and begin decode.
 *
 * Thus, the first action taken by applications dealing with such data is
 * to read in and cache the decode headers; thereafter the application can
 * safely seek to arbitrary points in the data.
 *
 * This impacts seeking because the portion of the bitstream containing
 * decode headers should not be considered part of the metric space. To
 * inform Oggz not to seek earlier than the end of the decode headers,
 * use oggz_data_begins_here().
 *
 * \{
 */

/**
 * Specify that a logical bitstream has a linear metric
 * \param oggz An OGGZ handle
 * \param serialno Identify the logical bitstream in \a oggz to attach
 * this linear metric to. A value of -1 indicates that the metric should
 * be attached to all unattached logical bitstreams in \a oggz.
 * \param granule_rate_numerator The numerator of the granule rate
 * \param granule_rate_denominator The denominator of the granule rate
 * \returns 0 Success
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 */
int oggz_set_metric_linear (OGGZ * oggz, long serialno,
			    ogg_int64_t granule_rate_numerator,
			    ogg_int64_t granule_rate_denominator);

/**
 * This is the signature of a function to correlate Ogg streams.
 * If every position in an Ogg stream can be described by a metric (eg. time)
 * then define this function that returns some arbitrary unit value.
 * This is the normal use of OGGZ for media streams. The meaning of units is
 * arbitrary, but must be consistent across all logical bitstreams; for
 * example a conversion of the time offset of a given packet into nanoseconds
 * or a similar stream-specific subdivision may be appropriate.
 *
 * \param oggz An OGGZ handle
 * \param serialno Identifies a logical bitstream within \a oggz
 * \param granulepos A granulepos within the logical bitstream identified
 *                   by \a serialno
 * \param user_data Arbitrary data you wish to pass to your callback
 * \returns A conversion of the (serialno, granulepos) pair into a measure
 * in units which is consistent across all logical bitstreams within \a oggz
 */
typedef ogg_int64_t (*OggzMetric) (OGGZ * oggz, long serialno,
				   ogg_int64_t granulepos, void * user_data);

/**
 * Set the OggzMetric to use for an OGGZ handle
 *
 * \param oggz An OGGZ handle
 * \param serialno Identify the logical bitstream in \a oggz to attach
 *                 this metric to. A value of -1 indicates that this metric
 *                 should be attached to all unattached logical bitstreams
 *                 in \a oggz.
 * \param metric An OggzMetric callback
 * \param user_data arbitrary data to pass to the metric callback
 *
 * \returns 0 Success
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 *                               logical bitstream in \a oggz, and is not -1
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 *
 * \note Specifying values of \a serialno other than -1 allows you to pass
 *       logical bitstream specific user_data to the same metric.
 * \note Alternatively, you may use a different \a metric for each
 *       \a serialno, but all metrics used must return mutually consistent
 *       unit measurements.
 */
int oggz_set_metric (OGGZ * oggz, long serialno, OggzMetric metric,
		     void * user_data);

/**
 * Seek to a number of units corresponding to the Metric function
 * \param oggz An OGGZ handle
 * \param units A number of units
 * \param whence As defined in <stdio.h>: SEEK_SET, SEEK_CUR or SEEK_END
 * \returns the new file offset, or -1 on failure.
 */
ogg_int64_t oggz_seek_units (OGGZ * oggz, ogg_int64_t units, int whence);

/** \}
 */

#ifdef _UNIMPLEMENTED
/** \defgroup order OggzOrder
 *
 * \subsection OggzOrder
 *
 * Suppose there is a partial order < and a corresponding equivalence
 * relation = defined on the space of packets in the Ogg stream of 'OGGZ'.
 * An OggzOrder simply provides a comparison in terms of '<' and '=' for
 * ogg_packets against a target.
 *
 * To use OggzOrder:
 *
 * - Implement an OggzOrder callback
 * - Set the OggzOrder callback for an OGGZ handle with oggz_set_order()
 * - To seek, use oggz_seek_byorder(). Oggz will use a combination bisection
 *   search and scan of the Ogg bitstream, using the OggzOrder callback to
 *   match against the desired 'target'.
 *
 * Otherwise, for more general ogg streams for which a partial order can be
 * defined, define a function matching this specification.
 *
 * Parameters:
 *
 *     OGGZ: the OGGZ object
 *     op:  an ogg packet in the stream
 *     target: a user defined object
 *
 * Return values:
 *
 *    -1 , if 'op' would occur before the position represented by 'target'
 *     0 , if the position of 'op' is equivalent to that of 'target'
 *     1 , if 'op' would occur after the position represented by 'target'
 *     2 , if the relationship between 'op' and 'target' is undefined.
 *
 * Symbolically:
 *
 * Suppose there is a partial order < and a corresponding equivalence
 * relation = defined on the space of packets in the Ogg stream of 'OGGZ'.
 * Let p represent the position of the packet 'op', and t be the position
 * represented by 'target'.
 *
 * Then a function implementing OggzPacketOrder should return as follows:
 *
 *    -1 , p < t
 *     0 , p = t
 *     1 , t < p
 *     2 , otherwise
 *
 * Hacker's hint: if there are no circumstances in which you would return
 * a value of 2, there is a linear order; it may be possible to define a
 * Metric rather than an Order.
 * \{
 */
typedef int (*OggzOrder) (OGGZ * oggz, ogg_packet * op, void * target,
			 void * user_data);
/**
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
int oggz_set_order (OGGZ * oggz, long serialno, OggzOrder order,
		    void * user_data);

long oggz_seek_byorder (OGGZ * oggz, void * target);

/** \}
 */
#endif /* _UNIMPLEMENTED */

/**
 * Tell OGGZ that we're past the headers, to remember the current packet
 * as the start of data.
 * This informs the seeking mechanism that when seeking back to unit 0,
 * go to the packet we're on now, not to the start of the file, which
 * is usually codec headers.
 * \param oggz An OGGZ handle previously opened for reading
 * \returns 0 on success, -1 on failure.
 */
int oggz_data_begins_here (OGGZ * oggz);

/**
 * Provide the file offset in bytes corresponding to the data read.
 * \param oggz An OGGZ handle
 * \returns The current offset of oggz.
 *
 * \note When reading, the value returned by oggz_tell() reflects the
 * data offset of the start of the most recent packet processed, so that
 * when called from an OggzReadPacket callback it reflects the byte
 * offset of the start of the packet. As OGGZ may have internally read
 * ahead, this may differ from the current offset of the associated file
 * descriptor.
 */
off_t oggz_tell (OGGZ * oggz);

/**
 * Seek to a specific bytes offset
 * \param oggz An OGGZ handle
 * \param offset a byte offset
 * \param whence As defined in <stdio.h>: SEEK_SET, SEEK_CUR or SEEK_END
 * \returns the new file offset, or -1 on failure.
 */
off_t oggz_seek (OGGZ * oggz, off_t offset, int whence);

#ifdef _UNIMPLEMENTED
long oggz_seek_packets (OGGZ * oggz, long serialno, long packets, int whence);
#endif


/**
 * Request a new serialno, as required for a new stream, ensuring the serialno
 * is not yet used for any other streams managed by this OGGZ.
 * \param oggz An OGGZ handle
 * \returns A new serialno, not already occuring in any logical bitstreams
 * in \a oggz.
 */
long oggz_serialno_new (OGGZ * oggz);


#endif /* __OGGZ_H__ */

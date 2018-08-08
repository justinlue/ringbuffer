#pragma once

// /* Includes: */
// #include "types.h"
// #include "../mcu/irq_i.h"
// #include "../../vendor/common/msg_interface.h"

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
	extern "C" {
#endif

/* Type Defines: */
/** \brief Ring buffer Management Structure.
 *
 *  Type define for a new ring buffer object. buffers should be initialized via a call to
 *  \ref ringbuffer_init() before use.
 */

//#include "opple_msg_interface.h"
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#pragma pack(1)
typedef struct {
    uint16_t addr;
    uint8_t *payload;
} OPPLE_QUEUE;
#pragma pack()

typedef struct
{
	OPPLE_QUEUE **in; /**< Current storage location in the circular buffer. */
	OPPLE_QUEUE **out; /**< Current retrieval location in the circular buffer. */
	OPPLE_QUEUE **start; /**< Pointer to the start of the buffer's underlying storage array. */
	OPPLE_QUEUE **end; /**< Pointer to the end of the buffer's underlying storage array. */
	uint16_t size; /**< size of the buffer's underlying storage array. */
	uint16_t sount; /**< Number of bytes currently stored in the buffer. */
} ringbuffer_t;

/* Inline Functions: */
/** Initializes a ring buffer ready for use. buffers must be initialized via this function
 *  before any operations are called upon them. Already initialized buffers may be reset
 *  by re-initializing them using this function.
 *
 *  \param[out] buffer   Pointer to a ring buffer structure to initialize.
 *  \param[out] dataptr  Pointer to a global array that will hold the data stored into the ring buffer.
 *  \param[out] size     Maximum number of bytes that can be stored in the underlying data array.
 */
static inline void ringbuffer_init(ringbuffer_t *buffer, OPPLE_QUEUE *dataptr[], const uint16_t size)
{
	// uint8_t r = irq_disable();

	buffer->in     = &dataptr[0];
	buffer->out    = &dataptr[0];
	buffer->start  = &dataptr[0];
	buffer->end    = &dataptr[size];
	buffer->size   = size;
	buffer->sount  = 0;

	// irq_restore(r);
}

/** Retrieves the current number of bytes stored in a particular buffer. This value is computed
 *  by entering an atomic lock on the buffer, so that the buffer cannot be modified while the
 *  computation takes place. This value should be cached when reading out the contents of the buffer,
 *  so that as small a time as possible is spent in an atomic lock.
 *
 *  \note The value returned by this function is guaranteed to only be the minimum number of bytes
 *        stored in the given buffer; this value may change as other threads write new data, thus
 *        the returned number should be used only to determine how many successive reads may safely
 *        be performed on the buffer.
 *
 *  \param[in] buffer  Pointer to a ring buffer structure whose count is to be computed.
 *
 *  \return Number of bytes currently stored in the buffer.
 */
static inline uint16_t ringbuffer_get_count(ringbuffer_t *const buffer)
{
	// u8 r = irq_disable();
	uint16_t sount = buffer->sount;
	// irq_restore(r);
	return sount;
}

/** Retrieves the free space in a particular buffer. This value is computed by entering an atomic lock
 *  on the buffer, so that the buffer cannot be modified while the computation takes place.
 *
 *  \note The value returned by this function is guaranteed to only be the maximum number of bytes
 *        free in the given buffer; this value may change as other threads write new data, thus
 *        the returned number should be used only to determine how many successive writes may safely
 *        be performed on the buffer when there is a single writer thread.
 *
 *  \param[in] buffer  Pointer to a ring buffer structure whose free count is to be computed.
 *
 *  \return Number of free bytes in the buffer.
 */
static inline uint16_t ringbuffer_get_free_count(ringbuffer_t *const buffer)
{
	return (buffer->size - ringbuffer_get_count(buffer));
}

/** Atomically determines if the specified ring buffer contains any data. This should
 *  be tested before removing data from the buffer, to ensure that the buffer does not
 *  underflow.
 *
 *  If the data is to be removed in a loop, store the total number of bytes stored in the
 *  buffer (via a call to the \ref ringbuffer_get_count() function) in a temporary variable
 *  to reduce the time spent in atomicity locks.
 *
 *  \param[in,out] buffer  Pointer to a ring buffer structure to insert into.
 *
 *  \return Boolean \c true if the buffer contains no free space, false otherwise.
 */
static inline uint8_t ringbuffer_is_empty(ringbuffer_t *const buffer)
{
	return (ringbuffer_get_count(buffer) == 0);
}

/** Atomically determines if the specified ring buffer contains any free space. This should
 *  be tested before storing data to the buffer, to ensure that no data is lost due to a
 *  buffer overrun.
 *
 *  \param[in,out] buffer  Pointer to a ring buffer structure to insert into.
 *
 *  \return Boolean \c true if the buffer contains no free space, false otherwise.
 */
static inline uint8_t ringbuffer_is_full(ringbuffer_t *const buffer)
{
	return (ringbuffer_get_count(buffer) == buffer->size);
}

/** Inserts an element into the ring buffer.
 *
 *  \note Only one execution thread (main program thread or an ISR) may insert into a single buffer
 *        otherwise data corruption may occur. Insertion and removal may occur from different execution
 *        threads.
 *
 *  \param[in,out] buffer  Pointer to a ring buffer structure to insert into.
 *  \param[in]     Data    Data element to insert into the buffer.
 */
static inline void ringbuffer_insert(ringbuffer_t *buffer, const OPPLE_QUEUE *Data)
{
	*buffer->in = Data;
//	memcpy(buffer->in, Data, sizeof(OPPLE_QUEUE));
printf("in: %x, end: %x\n", buffer->in, buffer->end);
	if (++ buffer->in == buffer->end) {
		buffer->in = buffer->start;
	}
	// u8 r = irq_disable();

	buffer->sount ++;

	// irq_restore(r);
}

/** Removes an element from the ring buffer.
 *
 *  \note Only one execution thread (main program thread or an ISR) may remove from a single buffer
 *        otherwise data corruption may occur. Insertion and removal may occur from different execution
 *        threads.
 *
 *  \param[in,out] buffer  Pointer to a ring buffer structure to retrieve from.
 *
 *  \return Next data element stored in the buffer.
 */
static inline OPPLE_QUEUE *ringbuffer_remove(ringbuffer_t *buffer)
{
	OPPLE_QUEUE *Data = *buffer->out;

	if (++ buffer->out == buffer->end) {
	  buffer->out = buffer->start;
	}
	// u8 r = irq_disable();

	buffer->sount --;

	// irq_restore(r);

	return Data;
}

/** Returns the next element stored in the ring buffer, without removing it.
 *
 *  \param[in,out] buffer  Pointer to a ring buffer structure to retrieve from.
 *
 *  \return Next data element stored in the buffer.
 */
static inline OPPLE_QUEUE *ringbuffer_peek(ringbuffer_t *const buffer)
{
	return *buffer->out;
}

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

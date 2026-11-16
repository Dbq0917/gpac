#include "tests.h"
#include <gpac/internal/isomedia_dev.h>

static GF_Err parse_full_box_with_large_payload(u32 type, u64 payload_size)
{
	const u64 box_size = 16 + 4 + payload_size;
	GF_BitStream *wbs, *bs;
	GF_Box *box = NULL;
	GF_Err e;
	u8 *data = NULL;
	u32 size = 0, log_level;

	wbs = gf_bs_new(NULL, 0, GF_BITSTREAM_WRITE);
	if (!wbs) return GF_OUT_OF_MEM;
	gf_bs_write_u32(wbs, 1);
	gf_bs_write_u32(wbs, type);
	gf_bs_write_u64(wbs, box_size);
	gf_bs_write_u32(wbs, 0);
	gf_bs_get_content(wbs, &data, &size);
	gf_bs_del(wbs);
	if (!data) return GF_OUT_OF_MEM;

	/* The logical extent is the full declared box size, as with the sparse
	 * reproducer; only the header needs to be materialized for this check. */
	bs = gf_bs_new(data, box_size, GF_BITSTREAM_READ);
	if (!bs) {
		gf_free(data);
		return GF_OUT_OF_MEM;
	}

	log_level = gf_log_get_tool_level(GF_LOG_CONTAINER);
	gf_log_set_tool_level(GF_LOG_CONTAINER, GF_LOG_QUIET);
	e = gf_isom_box_parse(&box, bs);
	gf_log_set_tool_level(GF_LOG_CONTAINER, (GF_LOG_Level) log_level);

	gf_bs_del(bs);
	gf_free(data);
	if (box) gf_isom_box_del(box);
	return e;
}

unittest(isom_reject_truncated_string_box_sizes)
{
	assert_equal(parse_full_box_with_large_payload(GF_ISOM_BOX_TYPE_KIND, 0x100000000ULL), GF_ISOM_INVALID_FILE, "%d");
	assert_equal(parse_full_box_with_large_payload(GF_ISOM_BOX_TYPE_SVHD, 0x100000000ULL), GF_ISOM_INVALID_FILE, "%d");
	assert_equal(parse_full_box_with_large_payload(GF_ISOM_BOX_TYPE_TXTC, 0x100000000ULL), GF_ISOM_INVALID_FILE, "%d");
	/* cprt consumes 6 bytes before its string allocation. */
	assert_equal(parse_full_box_with_large_payload(GF_ISOM_BOX_TYPE_CPRT, (u64)GF_UINT_MAX + 6), GF_ISOM_INVALID_FILE, "%d");
	assert_equal(parse_full_box_with_large_payload(GF_ISOM_BOX_TYPE_ELNG, GF_UINT_MAX), GF_ISOM_INVALID_FILE, "%d");
}

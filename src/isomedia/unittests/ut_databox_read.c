#include "tests.h"
#include <gpac/internal/isomedia_dev.h>

GF_Err databox_box_read(GF_Box *s, GF_BitStream *bs);

static GF_Err databox_read_payload(GF_DataBox *ptr, u64 declared_payload_size, const u8 *bytes, u32 bytes_len)
{
	GF_Err e;
	GF_BitStream *bs;
	/* size as the generic parser would leave it: locale plus declared payload */
	((GF_Box *)ptr)->size = 4 + declared_payload_size;
	bs = gf_bs_new((u8 *) bytes, bytes_len, GF_BITSTREAM_READ);
	if (!bs) return GF_OUT_OF_MEM;
	e = databox_box_read((GF_Box *)ptr, bs);
	gf_bs_del(bs);
	return e;
}

unittest(databox_read_size_validation)
{
	GF_Err e;
	GF_DataBox *ptr;
	u32 log_level = gf_log_get_tool_level(GF_LOG_CONTAINER);
	u32 core_log_level = gf_log_get_tool_level(GF_LOG_CORE);
	/* locale (4 bytes) followed by the "SomeTitle" payload */
	u8 valid_content[4+9] = {0, 0, 0, 0, 'S', 'o', 'm', 'e', 'T', 'i', 't', 'l', 'e'};
	/* locale plus only 10 bytes of backing data, declared payload says 100 */
	u8 short_content[4+10] = {0};

	/* declared payload above 32 bits must be rejected, not truncated */
	ptr = (GF_DataBox *) gf_isom_box_new(GF_ISOM_BOX_TYPE_DATA);
	assert_not_null(ptr);
	if (!ptr) return;
	gf_log_set_tool_level(GF_LOG_CONTAINER, GF_LOG_QUIET);
	e = databox_read_payload(ptr, 0x100000000ULL, valid_content, sizeof(valid_content));
	gf_log_set_tool_level(GF_LOG_CONTAINER, (GF_LOG_Level) log_level);
	assert_equal(e, GF_ISOM_INVALID_FILE, "%d");
	assert_true(ptr->data == NULL);
	assert_equal(ptr->dataSize, 0, "%u");
	gf_isom_box_del((GF_Box *)ptr);

	/* declared payload of 0xFFFFFFFF must not wrap the u32 data size */
	ptr = (GF_DataBox *) gf_isom_box_new(GF_ISOM_BOX_TYPE_DATA);
	assert_not_null(ptr);
	if (!ptr) return;
	gf_log_set_tool_level(GF_LOG_CONTAINER, GF_LOG_QUIET);
	e = databox_read_payload(ptr, 0xFFFFFFFFUL, valid_content, sizeof(valid_content));
	gf_log_set_tool_level(GF_LOG_CONTAINER, (GF_LOG_Level) log_level);
	assert_equal(e, GF_ISOM_INVALID_FILE, "%d");
	assert_true(ptr->data == NULL);
	assert_equal(ptr->dataSize, 0, "%u");
	gf_isom_box_del((GF_Box *)ptr);

	/* a short read must be an explicit rejection, not uninitialized data */
	ptr = (GF_DataBox *) gf_isom_box_new(GF_ISOM_BOX_TYPE_DATA);
	assert_not_null(ptr);
	if (!ptr) return;
	gf_log_set_tool_level(GF_LOG_CONTAINER, GF_LOG_QUIET);
	gf_log_set_tool_level(GF_LOG_CORE, GF_LOG_QUIET);
	e = databox_read_payload(ptr, 100, short_content, sizeof(short_content));
	gf_log_set_tool_level(GF_LOG_CONTAINER, (GF_LOG_Level) log_level);
	gf_log_set_tool_level(GF_LOG_CORE, (GF_LOG_Level) core_log_level);
	assert_equal(e, GF_ISOM_INVALID_FILE, "%d");
	gf_isom_box_del((GF_Box *)ptr);

	/* valid control: a well-formed box parses with an unchanged value */
	ptr = (GF_DataBox *) gf_isom_box_new(GF_ISOM_BOX_TYPE_DATA);
	assert_not_null(ptr);
	if (!ptr) return;
	e = databox_read_payload(ptr, 9, valid_content, sizeof(valid_content));
	assert_equal(e, GF_OK, "%d");
	assert_equal(ptr->dataSize, 9, "%u");
	assert_not_null(ptr->data);
	if (ptr->data) {
		assert_equal_mem(ptr->data, "SomeTitle", 9);
		assert_equal(ptr->data[9], 0, "%d");
	}
	gf_isom_box_del((GF_Box *)ptr);
}

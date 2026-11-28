#include <stdio.h>
#include <string.h>
#include <pa220_install_plan.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); failures++; } else assertions++; } while (0)
static int assertions, failures;

static void setup(struct pa220_install_manifest *m,
                  struct pa220_partition_table *t,
                  struct pa220_plan_component *c,
                  struct pa220_target_selection *s)
{
    memset(m,0,sizeof(*m)); memset(t,0,sizeof(*t));
    memset(c,0,2*sizeof(*c)); memset(s,0,2*sizeof(*s));
    m->state=PA220_MANIFEST_APPROVED; m->board_type=PA220_PLAN_BOARD_TYPE;
    strcpy(m->manifest_id,"host-fixture-reviewed"); m->component_count=2;
    strcpy(m->components[0].name,"kernel");
    m->components[0].format_mask=PA220_PLAN_FORMAT_FIT;
    m->components[0].allowed_kind_mask=PA220_PLAN_KIND_PRIMARY;
    m->components[0].allowed_mbr_type=0x83;
    m->components[0].min_bytes=1; m->components[0].max_bytes=1024*1024;
    strcpy(m->components[1].name,"rootfs");
    m->components[1].format_mask=PA220_PLAN_FORMAT_FIT;
    m->components[1].allowed_kind_mask=PA220_PLAN_KIND_PRIMARY;
    m->components[1].allowed_mbr_type=0x83;
    m->components[1].min_bytes=1; m->components[1].max_bytes=1024*1024;
    t->status=PA220_PT_OK; t->format=PA220_PT_DOS_MBR;
    t->capacity=57802752; t->count=2;
    t->parts[0].index=1; t->parts[0].kind=PA220_PART_PRIMARY;
    t->parts[0].type=0x83; t->parts[0].start_lba=16; t->parts[0].sectors=3952;
    t->parts[1].index=2; t->parts[1].kind=PA220_PART_PRIMARY;
    t->parts[1].type=0x83; t->parts[1].start_lba=3968; t->parts[1].sectors=8000064;
    strcpy(c[0].name,"kernel"); c[0].format_mask=PA220_PLAN_FORMAT_FIT;
    c[0].source_offset=0x1000; c[0].size_bytes=4096; c[0].integrity_verified=1;
    strcpy(c[1].name,"rootfs"); c[1].format_mask=PA220_PLAN_FORMAT_FIT;
    c[1].source_offset=0x3000; c[1].size_bytes=8193; c[1].integrity_verified=1;
    strcpy(s[0].component_name,"kernel"); s[0].partition_index=1;
    strcpy(s[1].component_name,"rootfs"); s[1].partition_index=2;
}

int main(void)
{
    struct pa220_install_manifest m;
    struct pa220_partition_table t;
    struct pa220_plan_component c[2];
    struct pa220_target_selection s[2];
    struct pa220_install_plan p;

    setup(&m,&t,c,s);
    CHECK(pa220_install_plan_build(NULL,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_NO_APPROVED_MANIFEST);
    m.state=PA220_MANIFEST_PRESENT_UNREVIEWED;
    CHECK(pa220_install_plan_build(&m,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_NO_APPROVED_MANIFEST);
    m.state=PA220_MANIFEST_APPROVED;
    CHECK(pa220_install_plan_build(&m,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_OK);
    CHECK(p.item_count==2 && p.items[0].partition_index==1 && p.items[1].partition_index==2);
    CHECK(p.items[1].required_sectors==17);
    c[0].integrity_verified=0;
    CHECK(pa220_install_plan_build(&m,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_BAD_COMPONENT);
    c[0].integrity_verified=1;
    c[0].source_offset=UINT64_MAX-10; c[0].size_bytes=20;
    CHECK(pa220_install_plan_build(&m,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_BAD_COMPONENT);
    c[0].source_offset=0x1000; c[0].size_bytes=4096;
    c[1].source_offset=0x1800;
    CHECK(pa220_install_plan_build(&m,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_OVERLAP);
    c[1].source_offset=0x3000;
    s[1].partition_index=1;
    CHECK(pa220_install_plan_build(&m,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_BAD_SELECTION);
    s[1].partition_index=2;
    t.parts[0].sectors=1;
    CHECK(pa220_install_plan_build(&m,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_CAPACITY);
    t.parts[0].sectors=3952;
    t.parts[1].type=0x82;
    CHECK(pa220_install_plan_build(&m,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_TARGET_MISMATCH);
    t.parts[1].type=0x83;
    m.board_type=28;
    CHECK(pa220_install_plan_build(&m,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_BAD_MANIFEST);

    setup(&m,&t,c,s);
    t.format=PA220_PT_GPT; t.parts[0].kind=PA220_PART_GPT;
    t.parts[1].kind=PA220_PART_GPT;
    memset(t.parts[0].type_guid,0x11,16);
    memset(t.parts[1].type_guid,0x22,16);
    memset(m.components[0].gpt_type_guid,0x11,16);
    memset(m.components[1].gpt_type_guid,0x22,16);
    m.components[0].allowed_kind_mask=PA220_PLAN_KIND_GPT;
    m.components[1].allowed_kind_mask=PA220_PLAN_KIND_GPT;
    m.components[0].allowed_mbr_type=0; m.components[1].allowed_mbr_type=0;
    CHECK(pa220_install_plan_build(&m,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_OK);
    memset(m.components[1].gpt_type_guid,0x33,16);
    CHECK(pa220_install_plan_build(&m,&t,0x10000,c,2,s,2,&p)==PA220_PLAN_TARGET_MISMATCH);

    printf("P7_3B_ASSERTIONS=%d\nP7_3B_FAILURES=%d\n",
           assertions,failures);
    return failures ? 1 : 0;
}

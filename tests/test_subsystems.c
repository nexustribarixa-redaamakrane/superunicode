#include <assert.h>
#include <string.h>
#include "suas/suas_sucha.h"
#include "suas/suas_stx.h"
#include "suas/suas_sktc.h"
#include "suts/suts_suicl.h"
#include "suts/suts_sulp.h"
#include "suts/suts_strs.h"
#include "sutr/sutr_sie.h"
#include "sutr/sutr_sfs.h"
#include "sutr/sutr_sim.h"
#include "sutr/sutr_skc.h"

int main(void)
{
    suas_stx_state_t stx;
    suas_sktc_report_t report;
    sucs_ex_char_t id[] = { 'a', 'b' };
    sutr_sie_queue_t events;
    sutr_sim_channel_t channel;
    sutr_skc_buffer_t console;
    uint32_t event;
    char payload[8];
    size_t length;

    assert(suas_sucha_version_string() != NULL);
    {
        sucd_record_t record;
        (void)suas_sucha_query('A', &record);
        (void)suas_sucha_property_mask('A');
    }
    suas_stx_init(&stx);
    assert(suas_stx_feed(&stx, 'a'));
    assert(suas_stx_flush(&stx) == SUAS_STS_BOUNDARY_MUST);
    assert(suts_suicl_validate(id, 2u));
    assert(suts_suicl_compare(id, 2u, id, 2u) == 0);
    assert(suts_sulp_version_string() != NULL);
    assert(suts_strs_width('A', false) == 1u);
    sutr_sie_init(&events);
    assert(sutr_sie_push(&events, 42u));
    assert(sutr_sie_pop(&events, &event) && event == 42u);
    assert(sutr_sfs_valid_name("file", 4u));
    assert(!sutr_sfs_valid_name("../file", 7u));
    sutr_sim_init(&channel);
    assert(sutr_sim_send(&channel, "ok", 2u));
    assert(sutr_sim_receive(&channel, payload, sizeof payload, &length));
    assert(length == 2u && memcmp(payload, "ok", 2u) == 0);
    sutr_skc_init(&console);
    assert(sutr_skc_write(&console, "boot", 4u));
    assert(console.length == 4u);
    suas_sktc_reset(&report);
    assert(suas_sktc_record(&report, true));
    assert(suas_sktc_complete(&report));
    return 0;
}

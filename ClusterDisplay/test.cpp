#include <string.h>
#include "zenoh.h"

int main(int argc, char **argv) {
    z_owned_config_t config;
    z_config_default(&config);
    z_owned_session_t s;
    if (z_open(&s, z_move(config)) != 0) {
        printf("Failed to open Zenoh session\n");
        exit(-1);
    }

    z_owned_bytes_t payload;
    z_bytes_from_static_str(&payload, "value");
    z_view_keyexpr_t key_expr;
    z_view_keyexpr_from_string(&key_expr, "key/expression");

    z_put(z_loan(s), z_loan(key_expr), z_move(payload), NULL);

    z_drop(z_move(s));
    return 0;
}

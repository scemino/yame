#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mo5_t* mo5;
} ui_mo5_desc_t;

typedef struct {
    mo5_t* mo5;
    ui_mc6809_t cpu;
    ui_kbd_t kbd;
} ui_mo5_t;

void ui_mo5_init(ui_mo5_t* ui, const ui_mo5_desc_t* desc);
void ui_mo5_discard(ui_mo5_t* ui);
void ui_mo5_draw(ui_mo5_t* ui);

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef CHIPS_UI_IMPL
static const ui_chip_pin_t _ui_mo5_cpu_pins[] = {
    { "D0",     0,      MC6809E_D0 },
    { "D1",     1,      MC6809E_D1 },
    { "D2",     2,      MC6809E_D2 },
    { "D3",     3,      MC6809E_D3 },
    { "D4",     4,      MC6809E_D4 },
    { "D5",     5,      MC6809E_D5 },
    { "D6",     6,      MC6809E_D6 },
    { "D7",     7,      MC6809E_D7 },
    { "NMI",    8,      MC6809E_PIN_NMI },
    { "IRQ",    9,      MC6809E_PIN_IRQ },
    { "FIRQ",   10,     MC6809E_PIN_FIRQ },
    { "HALT",   11,     MC6809E_PIN_HALT },
    { "RESET",  12,     MC6809E_PIN_RST },
    { "R/W",    13,     MC6809E_PIN_RW },
    { "A0",     14,     MC6809E_A0 },
    { "A1",     15,     MC6809E_A1 },
    { "A2",     16,     MC6809E_A2 },
    { "A3",     17,     MC6809E_A3 },
    { "A4",     18,     MC6809E_A4 },
    { "A5",     19,     MC6809E_A5 },
    { "A6",     20,     MC6809E_A6 },
    { "A7",     21,     MC6809E_A7 },
    { "A8",     22,     MC6809E_A8 },
    { "A9",     23,     MC6809E_A9 },
    { "A10",    24,     MC6809E_A10 },
    { "A11",    25,     MC6809E_A11 },
    { "A12",    26,     MC6809E_A12 },
    { "A13",    27,     MC6809E_A13 },
    { "A14",    28,     MC6809E_A14 },
    { "A15",    29,     MC6809E_A15 },
};

static void _ui_mo5_draw_menu(ui_mo5_t* ui) {
    CHIPS_ASSERT(ui && ui->mo5);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("System")) {
            if (ImGui::MenuItem("Reset")) {
                mo5_reset(ui->mo5);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Hardware")) {
            ImGui::MenuItem("Keyboard Matrix", 0, &ui->kbd.open);
            ImGui::MenuItem("MC6809 (CPU)", 0, &ui->cpu.open);
            ImGui::EndMenu();
        }
        //ui_util_options_menu();
        ImGui::EndMainMenuBar();
    }
}

void ui_mo5_init(ui_mo5_t* ui, const ui_mo5_desc_t* ui_desc) {
    CHIPS_ASSERT(ui && ui_desc);
    CHIPS_ASSERT(ui_desc->mo5);
    ui->mo5 = ui_desc->mo5;
    int x = 20, y = 20, dx = 10, dy = 10;
    {
        ui_mc6809_desc_t desc = {0};
        desc.title = "MC6809 CPU";
        desc.cpu = &ui->mo5->cpu;
        desc.x = x;
        desc.y = y;
        UI_CHIP_INIT_DESC(&desc.chip_desc, "MC6809\nCPU", 36, _ui_mo5_cpu_pins);
        ui_mc6809_init(&ui->cpu, &desc);
    }
    x += dx; y += dy;
    {
        ui_kbd_desc_t desc = {0};
        desc.title = "Keyboard Matrix";
        desc.kbd = &ui->mo5->kbd;
        desc.layers[0] = "None";
        desc.layers[1] = "Shift";
        desc.layers[2] = "Ctrl";
        desc.x = x;
        desc.y = y;
        ui_kbd_init(&ui->kbd, &desc);
    }
}

void ui_mo5_discard(ui_mo5_t* ui) {
    CHIPS_ASSERT(ui && ui->mo5);
    ui->mo5 = 0;
    ui_mc6809_discard(&ui->cpu);
}

void ui_mo5_draw(ui_mo5_t* ui) {
    CHIPS_ASSERT(ui && ui->mo5);
    _ui_mo5_draw_menu(ui);
    ui_kbd_draw(&ui->kbd);
    ui_mc6809_draw(&ui->cpu);
}
#endif

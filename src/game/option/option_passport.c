#include "game/option/option_passport.h"

#include "config.h"
#include "game/difficulty.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inventory/inventory_vars.h"
#include "game/requester.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/vars.h"
#include "memory.h"

#define MAX_GAME_MODES 4
#define MAX_GAME_MODE_LENGTH 20

typedef enum PASSPORT_TEXT {
    TEXT_PAGE_NAME = 0,
    TEXT_LEFT_ARROW = 1,
    TEXT_RIGHT_ARROW = 2,
    TEXT_LEVEL_ARROW_RIGHT = 3,
    TEXT_LEVEL_ARROW_LEFT = 4,
    TEXT_NUMBER_OF = 5,
} PASSPORT_TEXT;

static PASSPORT_MODE m_PassportMode = PASSPORT_MODE_FLIP;
static bool m_IsTextInit = false;
static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = { 0 };
static char m_NewGameStrings[MAX_GAME_MODES][MAX_GAME_MODE_LENGTH] = { 0 };

static char **m_SelectLevelStrings = NULL;
static char *m_SelectLevelBuffer = NULL;

static REQUEST_INFO m_NewGameRequester = {
    .items = MAX_GAME_MODES,
    .requested = 0,
    .vis_lines = MAX_GAME_MODES,
    .line_offset = 0,
    .line_old_offset = 0,
    .pix_width = 162,
    .line_height = TEXT_HEIGHT + 7,
    .x = 0,
    .y = 0,
    .flags = 0,
    .heading_text = NULL,
    .item_texts = &m_NewGameStrings[0][0],
    .item_text_len = MAX_GAME_MODE_LENGTH,
    0,
};

static REQUEST_INFO m_SelectLevelRequester = {
    .items = 1,
    .requested = 0,
    .vis_lines = -1,
    .line_offset = 0,
    .line_old_offset = 0,
    .pix_width = 272,
    .line_height = TEXT_HEIGHT + 7,
    .x = 0,
    .y = -32,
    .flags = 0,
    .heading_text = NULL,
    .item_texts = NULL,
    .item_text_len = MAX_LEVEL_NAME_LENGTH,
    0,
};

REQUEST_INFO g_SavegameRequester = {
    .items = 1,
    .requested = 0,
    .vis_lines = -1,
    .line_offset = 0,
    .line_old_offset = 0,
    .pix_width = 272,
    .line_height = TEXT_HEIGHT + 7,
    .x = 0,
    .y = -32,
    .flags = 0,
    .heading_text = NULL,
    .item_texts = NULL,
    .item_text_len = MAX_LEVEL_NAME_LENGTH,
    0,
};

static void Option_PassportInitText(void);
static void Option_PassportShutdownText(void);
static void Option_PassportShowNewGame(void);
static void Option_PassportShowSaves(void);
static void Option_PassportInitText(void);
static void Option_PassportInitNewGameRequester(void);
static void Option_PassportInitSaveRequester(int16_t page_num);
static void Option_PassportLoadGame(void);
static void Option_PassportSelectLevel(void);

void Option_PassportInit(void)
{
    g_SavegameRequester.item_texts = Memory_Alloc(
        g_Config.maximum_save_slots * g_SavegameRequester.item_text_len);
    m_SelectLevelRequester.item_texts = Memory_Alloc(
        (g_GameFlow.level_count + 1) * m_SelectLevelRequester.item_text_len);
}

void Option_PassportShutdown(void)
{
    Memory_FreePointer(&g_SavegameRequester.item_texts);
    Memory_FreePointer(&m_SelectLevelRequester.item_texts);
}

static void Option_PassportInitText(void)
{
    m_Text[TEXT_LEFT_ARROW] = Text_Create(-85, -15, "\200");
    Text_Hide(m_Text[TEXT_LEFT_ARROW], true);

    m_Text[TEXT_RIGHT_ARROW] = Text_Create(70, -15, "\201");
    Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);

    m_Text[TEXT_LEVEL_ARROW_LEFT] = Text_Create(0, 0, "\200");
    Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], true);

    m_Text[TEXT_LEVEL_ARROW_RIGHT] = Text_Create(0, 0, "\201");
    Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);

    m_Text[TEXT_PAGE_NAME] = Text_Create(0, -16, "");

    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_AlignBottom(m_Text[i], 1);
        Text_CentreH(m_Text[i], 1);
    }
    Difficulty_Select(DIFFICULTY_SELECT_INIT);
}

static void Option_PassportShutdownText(void)
{
    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_Remove(m_Text[i]);
        m_Text[i] = NULL;
    }
    m_PassportMode = PASSPORT_MODE_FLIP;
    m_IsTextInit = false;
    Difficulty_Select(DIFFICULTY_SELECT_SHUTDOWN);
}

static void Option_PassportShowNewGame(void)
{
    int32_t select = Requester_Display(&m_NewGameRequester);
    if (select) {
        if (select > 0) {
            switch (select - 1) {
            case 0:
                g_GameInfo.bonus_flag = 0;
                break;
            case 1:
                g_GameInfo.bonus_flag = GBF_NGPLUS;
                break;
            case 2:
                g_GameInfo.bonus_flag = GBF_JAPANESE;
                break;
            case 3:
                g_GameInfo.bonus_flag = GBF_JAPANESE | GBF_NGPLUS;
                break;
            default:
                g_GameInfo.bonus_flag = 0;
                break;
            }
            g_GameInfo.current_save_slot = -1;
            g_GameInfo.passport_mode = PASSPORT_MODE_NEW_GAME;
            g_GameInfo.save_initial_version = SAVEGAME_CURRENT_VERSION;
        } else if (
            g_InvMode != INV_SAVE_MODE && g_InvMode != INV_SAVE_CRYSTAL_MODE
            && g_InvMode != INV_LOAD_MODE) {
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
        }
        m_PassportMode = PASSPORT_MODE_FLIP;
    } else {
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };
    }
}

static void Option_PassportShowLevelSelect(void)
{
    int32_t select = Requester_Display(&m_SelectLevelRequester);
    if (select) {
        if (select - 1 + g_GameFlow.first_level_num
            == Savegame_GetLevelNumber(g_GameInfo.current_save_slot) + 1) {
            g_GameInfo.passport_mode = PASSPORT_MODE_STORY_SO_FAR;
        } else if (select > 0) {
            g_GameInfo.select_level_num =
                select - 1 + g_GameFlow.first_level_num;
            g_GameInfo.passport_mode = PASSPORT_MODE_SELECT_LEVEL;
        } else if (
            g_InvMode != INV_SAVE_MODE && g_InvMode != INV_SAVE_CRYSTAL_MODE
            && g_InvMode != INV_LOAD_MODE) {
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
        }
        m_PassportMode = PASSPORT_MODE_FLIP;
    } else {
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };
    }
}

static void Option_PassportShowSaves(void)
{
    int32_t select = Requester_Display(&g_SavegameRequester);
    if (select) {
        if (select > 0) {
            g_GameInfo.current_save_slot = select - 1;
            g_GameInfo.passport_mode = PASSPORT_MODE_SHOW_SAVES;
        } else {
            if (g_InvMode != INV_SAVE_MODE && g_InvMode != INV_SAVE_CRYSTAL_MODE
                && g_InvMode != INV_LOAD_MODE) {
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
            }
        }
        m_PassportMode = PASSPORT_MODE_FLIP;
    } else {
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };
    }
}

static void Option_PassportInitNewGameRequester(void)
{
    REQUEST_INFO *req = &m_NewGameRequester;
    Requester_Init(req);
    req->requested = 0;
    Requester_SetHeading(req, g_GameFlow.strings[GS_PASSPORT_SELECT_MODE]);
    Requester_AddItem(req, g_GameFlow.strings[GS_PASSPORT_MODE_NEW_GAME], 0);
    Requester_AddItem(
        req, g_GameFlow.strings[GS_PASSPORT_MODE_NEW_GAME_PLUS], 0);
    Requester_AddItem(req, g_GameFlow.strings[GS_PASSPORT_MODE_NEW_GAME_JP], 0);
    Requester_AddItem(
        req, g_GameFlow.strings[GS_PASSPORT_MODE_NEW_GAME_JP_PLUS], 0);
    req->vis_lines = MAX_GAME_MODES;

    // Title screen passport is at a different pitch.
    if (g_InvMode == INV_TITLE_MODE) {
        req->y = (-Screen_GetResHeightDownscaled() / 2.4)
            + (req->line_height * req->vis_lines + 1);
    } else {
        req->y = (-Screen_GetResHeightDownscaled() / 2)
            + (req->line_height * req->vis_lines);
    }
}

static void Option_PassportInitSelectLevelRequester(void)
{
    REQUEST_INFO *req = &m_SelectLevelRequester;
    req->flags |= RIF_BLOCKABLE;
    Requester_Init(req);
    Requester_SetHeading(req, g_GameFlow.strings[GS_PASSPORT_SELECT_LEVEL]);

    if (Screen_GetResHeightDownscaled() <= 240) {
        req->vis_lines = 5;
    } else if (Screen_GetResHeightDownscaled() <= 384) {
        req->vis_lines = 7;
    } else if (Screen_GetResHeightDownscaled() <= 480) {
        req->vis_lines = 10;
    } else {
        req->vis_lines = 12;
    }

    // Title screen passport is at a different pitch.
    if (g_InvMode == INV_TITLE_MODE) {
        req->y = (-Screen_GetResHeightDownscaled() / 2)
            + (req->line_height * req->vis_lines);
    } else {
        req->y = (-Screen_GetResHeightDownscaled() / 1.73)
            + (req->line_height * req->vis_lines);
    }

    Savegame_ScanAvailableLevels(req);
}

static void Option_PassportInitSaveRequester(int16_t page_num)
{
    REQUEST_INFO *req = &g_SavegameRequester;
    Requester_Init(req);
    Requester_SetHeading(
        req,
        g_GameFlow.strings
            [page_num == PASSPORT_PAGE_1 ? GS_PASSPORT_LOAD_GAME
                                         : GS_PASSPORT_SAVE_GAME]);

    if (Screen_GetResHeightDownscaled() <= 240) {
        req->vis_lines = 5;
    } else if (Screen_GetResHeightDownscaled() <= 384) {
        req->vis_lines = 7;
    } else if (Screen_GetResHeightDownscaled() <= 480) {
        req->vis_lines = 10;
    } else {
        req->vis_lines = 12;
    }

    // Title screen passport is at a different pitch.
    if (g_InvMode == INV_TITLE_MODE) {
        req->y = (-Screen_GetResHeightDownscaled() / 2)
            + (req->line_height * req->vis_lines);
    } else {
        req->y = (-Screen_GetResHeightDownscaled() / 1.73)
            + (req->line_height * req->vis_lines);
    }

    Savegame_ScanSavedGames();
}

static void Option_PassportLoadGame(void)
{
    if (!(g_SavegameRequester.item_flags[g_SavegameRequester.requested]
          & RIF_BLOCKED)
        || !(g_SavegameRequester.flags & RIF_BLOCKABLE)) {
        if (g_InputDB.right) {
            g_GameInfo.current_save_slot = g_SavegameRequester.requested;
            Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
            Requester_Remove(&g_SavegameRequester);
            Option_PassportInitSelectLevelRequester();
            m_PassportMode = PASSPORT_MODE_SELECT_LEVEL;
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
            Option_PassportShowLevelSelect();
        } else {
            Option_PassportShowSaves();
            if (m_PassportMode == PASSPORT_MODE_SHOW_SAVES) {
                Text_SetPos(
                    m_Text[TEXT_LEVEL_ARROW_RIGHT], 120,
                    g_SavegameRequester
                        .texts
                            [g_SavegameRequester.requested
                             - g_SavegameRequester.line_offset]
                        ->pos.y);
                Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], false);
            } else {
                Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
            }
        }
    } else {
        Option_PassportShowSaves();
        Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
    }

    if ((g_SavegameRequester.item_flags[g_SavegameRequester.requested]
         & RIF_BLOCKED)
        && (g_SavegameRequester.flags & RIF_BLOCKABLE)) {
        Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
    }
}

static void Option_PassportSelectLevel(void)
{
    if (g_InputDB.left) {
        Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], true);
        Requester_Remove(&m_SelectLevelRequester);
        Option_PassportInitSaveRequester(PASSPORT_PAGE_1);
        m_PassportMode = PASSPORT_MODE_SHOW_SAVES;
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };
        Option_PassportShowSaves();
    } else {
        Option_PassportShowLevelSelect();
        if (m_PassportMode == PASSPORT_MODE_SELECT_LEVEL) {
            Text_SetPos(
                m_Text[TEXT_LEVEL_ARROW_LEFT], -120,
                m_SelectLevelRequester
                    .texts
                        [m_SelectLevelRequester.requested
                         - m_SelectLevelRequester.line_offset]
                    ->pos.y);
            Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], false);
        } else {
            Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], true);
        }
    }
}

void Option_Passport(INVENTORY_ITEM *inv_item)
{
    if (!m_IsTextInit) {
        Text_Remove(g_InvItemText[IT_NAME]);
        g_InvItemText[IT_NAME] = NULL;
        Text_Remove(g_InvRingText);
        g_InvRingText = NULL;
        Option_PassportInitText();
        m_IsTextInit = true;
    }

    int16_t page = (inv_item->goal_frame - inv_item->open_frame) / 5;
    if ((inv_item->goal_frame - inv_item->open_frame) % 5) {
        page = PASSPORT_PAGE_FLIPPING;
    }

    if (g_InvMode == INV_SAVE_MODE || g_InvMode == INV_SAVE_CRYSTAL_MODE) {
        g_InputDB.left = 0;
        g_InputDB.right = 0;
    }

    Difficulty_Select(DIFFICULTY_SELECT_HIDEALL);

    switch (page) {
    case PASSPORT_PAGE_1:
        Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
        if (m_PassportMode == PASSPORT_MODE_SHOW_SAVES) {
            Option_PassportLoadGame();
        } else if (m_PassportMode == PASSPORT_MODE_SELECT_LEVEL) {
            Option_PassportSelectLevel();
        } else if (m_PassportMode == PASSPORT_MODE_FLIP) {
            Difficulty_Select(DIFFICULTY_SELECT_WAITING_INPUT);
            if (!g_SavedGamesCount || g_InvMode == INV_SAVE_MODE
                || g_InvMode == INV_SAVE_CRYSTAL_MODE) {
                g_InputDB = (INPUT_STATE) { 0, .right = 1 };
            } else {
                Text_ChangeText(
                    m_Text[TEXT_PAGE_NAME],
                    g_GameFlow.strings[GS_PASSPORT_LOAD_GAME]);
                if (g_InputDB.select || g_InvMode == INV_LOAD_MODE) {
                    g_SavegameRequester.flags |= RIF_BLOCKABLE;
                    Option_PassportInitSaveRequester(page);
                    m_PassportMode = PASSPORT_MODE_SHOW_SAVES;
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                    Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
                } else {
                    Text_Hide(m_Text[TEXT_RIGHT_ARROW], false);
                }
            }
        }
        break;

    case PASSPORT_PAGE_2:
        if (m_PassportMode == PASSPORT_MODE_NEW_GAME) {
            Option_PassportShowNewGame();
        } else if (m_PassportMode == PASSPORT_MODE_SHOW_SAVES) {
            Option_PassportShowSaves();
        } else if (m_PassportMode == PASSPORT_MODE_FLIP) {
            Difficulty_Select(DIFFICULTY_SELECT_WAITING_INPUT);
            Text_Hide(m_Text[TEXT_LEFT_ARROW], false);
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], false);
            if (g_SavedGamesCount == 0) {
                g_InputDB.left = 0;
                Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
            }

            if (g_InvMode == INV_TITLE_MODE
                || g_CurrentLevel == g_GameFlow.gym_level_num) {
                Text_ChangeText(
                    m_Text[TEXT_PAGE_NAME],
                    g_GameFlow.strings[GS_PASSPORT_NEW_GAME]);
            } else if (g_InvMode == INV_DEATH_MODE) {
                if (Savegame_RestartAvailable(g_GameInfo.current_save_slot)) {
                    Text_ChangeText(
                        m_Text[TEXT_PAGE_NAME],
                        g_GameFlow.strings[GS_PASSPORT_RESTART_LEVEL]);
                } else {
                    g_InputDB = inv_item->anim_direction == 1
                        ? (INPUT_STATE) { 0, .right = 1 }
                        : (INPUT_STATE) { 0, .left = 1 };
                }
            } else {
                Difficulty_Select(DIFFICULTY_SELECT_HIDEALL);
                Text_ChangeText(
                    m_Text[TEXT_PAGE_NAME],
                    g_GameFlow.strings[GS_PASSPORT_SAVE_GAME]);
            }

            if (g_InputDB.select || g_InvMode == INV_SAVE_MODE
                || g_InvMode == INV_SAVE_CRYSTAL_MODE) {
                Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
                Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
                if (g_InvMode == INV_TITLE_MODE
                    || g_CurrentLevel == g_GameFlow.gym_level_num) {
                    if (g_GameFlow.enable_game_modes) {
                        Option_PassportInitNewGameRequester();
                        m_PassportMode = PASSPORT_MODE_NEW_GAME;
                        g_Input = (INPUT_STATE) { 0 };
                        g_InputDB = (INPUT_STATE) { 0 };
                    } else {
                        g_GameInfo.save_initial_version =
                            SAVEGAME_CURRENT_VERSION;
                        g_GameInfo.bonus_flag = 0;
                    }
                } else if (
                    g_InvMode == INV_SAVE_MODE
                    || g_InvMode == INV_SAVE_CRYSTAL_MODE
                    || g_InvMode == INV_GAME_MODE) {
                    g_SavegameRequester.flags &= ~RIF_BLOCKABLE;
                    Option_PassportInitSaveRequester(page);
                    m_PassportMode = PASSPORT_MODE_SHOW_SAVES;
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                }
            }
        }
        break;

    case PASSPORT_PAGE_3:
        Text_Hide(m_Text[TEXT_LEFT_ARROW], false);
        Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
        if (g_InvMode == INV_TITLE_MODE) {
            Text_ChangeText(
                m_Text[TEXT_PAGE_NAME],
                g_GameFlow.strings[GS_PASSPORT_EXIT_GAME]);
        } else {
            Text_ChangeText(
                m_Text[TEXT_PAGE_NAME],
                g_GameFlow.strings[GS_PASSPORT_EXIT_TO_TITLE]);
        }
        break;
    }

    bool pages_available[PASSPORT_PAGE_COUNT] = {
        g_SavedGamesCount > 0,
        g_InvMode == INV_TITLE_MODE || g_InvMode == INV_SAVE_CRYSTAL_MODE
            || !g_GameFlow.enable_save_crystals,
        true,
    };

    if (g_InputDB.left && (g_SavedGamesCount || page > PASSPORT_PAGE_1)) {
        while (--page >= PASSPORT_PAGE_1) {
            if (pages_available[page]) {
                break;
            }
        }

        if (page >= PASSPORT_PAGE_1) {
            inv_item->anim_direction = -1;
            inv_item->goal_frame = inv_item->open_frame + 5 * page;
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
        }

        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };
    }

    if (g_InputDB.right) {
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };

        while (++page < PASSPORT_PAGE_COUNT) {
            if (pages_available[page]) {
                break;
            }
        }

        if (page < PASSPORT_PAGE_COUNT) {
            inv_item->anim_direction = 1;
            inv_item->goal_frame = inv_item->open_frame + 5 * page;
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
        }
    }

    if (g_InputDB.deselect) {
        if (g_InvMode == INV_DEATH_MODE) {
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
        } else {
            if (page == PASSPORT_PAGE_2) {
                inv_item->anim_direction = 1;
                inv_item->goal_frame = inv_item->frames_total - 1;
            } else {
                inv_item->goal_frame = 0;
                inv_item->anim_direction = -1;
            }
            Option_PassportShutdownText();
        }
    }

    if (g_InputDB.select) {
        g_GameInfo.passport_page = page;
        if (page == PASSPORT_PAGE_3) {
            inv_item->anim_direction = 1;
            inv_item->goal_frame = inv_item->frames_total - 1;
        } else {
            inv_item->goal_frame = 0;
            inv_item->anim_direction = -1;
        }
        if (page != PASSPORT_PAGE_3
            && !(
                page == PASSPORT_PAGE_2 && g_InvMode == INV_GAME_MODE
                && g_GameInfo.passport_mode == PASSPORT_MODE_SHOW_SAVES)) {
            Difficulty_Select(DIFFICULTY_SELECT_CONFIRM);
        }
        Option_PassportShutdownText();
    }
}

#include "plugin.h"
#include "CFont.h"
#include "CTimer.h"
#include "CCamera.h"
#include "extensions/ScriptCommands.h"
#include "CTxdStore.h"
#include "CMessages.h"
#include "CCoronas.h"
#include "CSprite.h"
#include "CModelInfo.h"
#include "CFileMgr.h"
#include "map"

using namespace plugin;

// GV1

CVector2D mousePos = CVector2D(0.0f, 0.0f);
//std::fstream lg;
std::list< std::list<const char*>> ListPatterns;
//std::list<char*> screenLogMessages;
static CSprite2d mouseSprite;
bool bMouseDown = false;

bool menu_showGUI = false;
int menu_editingModelIndex = 0;
int menu_onScreen = 0;
int menu_editingCorona = 0;
int menu_subPage = 0;

// Functions 1

void InitPatterns() {
    std::list<const char*> pattern;

    pattern.push_back("1,1,0,160");
    pattern.push_back("0,0,0,50");
    pattern.push_back("0,0,1,160");
    pattern.push_back("0,0,0,50");

    ListPatterns.push_back(pattern);
    pattern.clear();

    pattern.push_back("1,1,1,60");
    pattern.push_back("0,0,0,60");
    pattern.push_back("1,1,1,60");
    pattern.push_back("0,0,0,300");

    ListPatterns.push_back(pattern);
    pattern.clear();

    pattern.push_back("1,1,0,60");
    pattern.push_back("0,0,0,60");
    pattern.push_back("1,1,0,60");
    pattern.push_back("0,0,0,300");
    pattern.push_back("0,1,1,60");
    pattern.push_back("0,0,0,60");
    pattern.push_back("0,1,1,60");
    pattern.push_back("0,0,0,300");

    ListPatterns.push_back(pattern);
    pattern.clear();

    pattern.push_back("1,1,0,80");
    pattern.push_back("0,0,0,60");
    pattern.push_back("1,1,0,80");
    pattern.push_back("0,0,0,60");
    pattern.push_back("1,1,0,80");
    pattern.push_back("0,0,0,300");
    pattern.push_back("0,1,1,80");
    pattern.push_back("0,0,0,60");
    pattern.push_back("0,1,1,80");
    pattern.push_back("0,0,0,60");
    pattern.push_back("0,1,1,80");
    pattern.push_back("0,0,0,300");

    ListPatterns.push_back(pattern);
    pattern.clear();
}

int GetNumberOfSteps(int patternId) {
    int i = 0;
    std::list< std::list<const char*>>::iterator it;
    for (it = ListPatterns.begin(); it != ListPatterns.end(); ++it) {
        if (i == patternId) {
            return (*it).size();
        }
        i++;
    }
    return 0;
}

void GetPatternStep(int patternId, int step, bool& left, bool& middle, bool& right, int& delay) {

    int i[2] = { 0, 0 };
    std::list< std::list<const char*>>::iterator it;
    for (it = ListPatterns.begin(); it != ListPatterns.end(); ++it) {
        if (i[0] == patternId) {
            break;
        }
        i[0]++;
    }

    std::list<const char*>::iterator it_;
    for (it_ = (*it).begin(); it_ != (*it).end(); ++it_) {
        if (i[1] == step) {
            break;
        }
        i[1]++;
    }

    int str_i = 0;

    size_t pos = 0;
    std::string token;
    std::string str = (*it_);
    while ((pos = str.find(",")) != std::string::npos) {
        token = str.substr(0, pos);
        int val = atoi(token.c_str());

        if (str_i == 0) { left = val; }
        if (str_i == 1) { middle = val; }
        if (str_i == 2) { right = val; }

        str.erase(0, pos + 1);
        str_i++;
    }
    delay = atoi(str.c_str());
}

float ScreenPosX(float x) {
    return (x * (SCREEN_COORD_CENTER_X * 2)) / 800.0f;
}

float ScreenPosY(float y) {
    return (y * (SCREEN_COORD_CENTER_Y * 2)) / 600.0f;
}

float clampf(float n, float lower, float upper) {
    return (std::max)(lower, (std::min)(n, upper));
}

float GetAngleBetweenVectors(CVector v1, CVector v2, CVector v3) {
    float v12 = sqrt(pow(v1.x - v2.x, 2) + pow(v1.y - v2.y, 2));
    float v13 = sqrt(pow(v1.x - v3.x, 2) + pow(v1.y - v3.y, 2));
    float v23 = sqrt(pow(v2.x - v3.x, 2) + pow(v2.y - v3.y, 2));
    return acos((pow(v12, 2) + pow(v13, 2) - pow(v23, 2)) / (2 * v12 * v13));
}

// Classes


class Input {
public:
    int type = 0;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool show = false;

    bool mouseOver, mouseOverL, mouseOverR = false;

    char stringText[256];
    char floatTitle[256];
    
    float floatValue = 0.0;
    float floatMin = 0.0;
    float floatMax = 0.0;

    int selection_current = 1;
    int selection_min = 1;
    int selection_max = 2;

    bool holdToAdd = false;

    CRGBA bgColor = CRGBA(122, 122, 122, 255);
    CRGBA bgColorHover = CRGBA(73, 73, 73, 255);
    CRGBA bgColorDisabled = CRGBA(20, 20, 20, 255);
    
    bool useIcon = false;
    CRGBA iconMainColor;
    CRGBA iconSecondaryColor;
    bool iconUseSecondary = false;

    Input(int type, float x, float y, float w, float h) {
        this->type = type;
        this->x = x;
        this->y = y;
        this->w = w;
        this->h = h;
    }

    void SetCoronaIcon(CRGBA mainColor, bool useSecondary, CRGBA secodarycolor) {
        useIcon = true;
        iconMainColor = mainColor;
        iconSecondaryColor = secodarycolor;
        iconUseSecondary = useSecondary;
    }

    void SetAsHoldToAdd() {
        holdToAdd = true;
    }

    void SetBackgroundColor(CRGBA color) { bgColor = color; }
    void SetBackgroundColorHover(CRGBA color) { bgColorHover = color; }

    void SetText(char* text) {
        sprintf(stringText, "%s", text);
    }

    void SetFloatEditorSettings(char* text, float min, float max) {
        sprintf(floatTitle, "%s", text);
        floatMin = min;
        floatMax = max;
    }

    void SetSelection(int value, int min, int max) {
        selection_current = value;
        selection_min = min;
        selection_max = max;
    }

    void Draw() {        
        float sx = ScreenPosX(x);
        float sy = ScreenPosY(y);
        float sw = ScreenPosX(w);
        float sh = ScreenPosY(h);

        CFont::SetOrientation(ALIGN_CENTER);
        CFont::SetDropShadowPosition(1);
        CFont::SetBackground(false, false);
        CFont::SetScale(ScreenPosX(0.3), ScreenPosY(0.9));
        CFont::SetFontStyle(FONT_SUBTITLES);
        CFont::SetProportional(true);
        CFont::SetColor(CRGBA(255, 255, 255, 255));

        if (type == 0 || type == 2) {
            mouseOver = (mousePos.x >= sx && mousePos.x < (sx + sw) && mousePos.y >= sy && mousePos.y < (sy + sh));
            CSprite2d::DrawRect(CRect(sx, sy, sx + sw, sy + sh), mouseOver ? bgColorHover : bgColor);
            if (type == 2) {
                sprintf(stringText, "%.2f", floatValue);
            }
            CFont::PrintString(sx + sw / 2, sy + ScreenPosY(1.0f), stringText);


            if (useIcon) {
                CRect rectMain = CRect(sx + 3, sy + 3, sx + sh - 3, sy + sh - 3);
                CSprite2d::DrawRect(rectMain, CRGBA(0, 0, 0, 255));
                CSprite2d::DrawRect(CRect(rectMain.left+2, rectMain.top+2, rectMain.right-2, rectMain.bottom-2), iconMainColor);

                if (iconUseSecondary) {
                    CRect rectSecondary = CRect(rectMain.right + 6, rectMain.top, rectMain.right + sh, rectMain.bottom);

                    CSprite2d::DrawRect(rectSecondary, CRGBA(0, 0, 0, 255));
                    CSprite2d::DrawRect(CRect(rectSecondary.left + 2, rectSecondary.top + 2, rectSecondary.right - 2, rectSecondary.bottom - 2), iconSecondaryColor);
                }
            }
            
        }

        if (type == 1) {
            CRect buttonLess = CRect(sx, sy, sx+sh, sy+sh);

            CRect inputVal = CRect(buttonLess.right + 5, sy, buttonLess.right + 5 + sw, sy+sh);

            CRect buttonMore = CRect(inputVal.right + 5, sy, inputVal.right + 5 + sh, sy + sh);

            mouseOverL = (mousePos.x >= buttonLess.left && mousePos.x < buttonLess.right && mousePos.y >= buttonLess.top && mousePos.y < buttonLess.bottom);
            mouseOverR = (mousePos.x >= buttonMore.left && mousePos.x < buttonMore.right && mousePos.y >= buttonMore.top && mousePos.y < buttonMore.bottom);

            CSprite2d::DrawRect(buttonLess, (selection_min == selection_current) ? bgColorDisabled : (mouseOverL ? bgColorHover : bgColor));

            CSprite2d::DrawRect(inputVal, CRGBA(68, 68, 68, 255));

            CSprite2d::DrawRect(buttonMore, (selection_max == selection_current) ? bgColorDisabled : (mouseOverR ? bgColorHover : bgColor));

            CFont::PrintString(inputVal.left + sw / 2, sy + ScreenPosY(1.0f), stringText);
        }

        if (type == 2) {
            //CSprite2d::DrawCircleAtNearClip(CVector2D(sx, sy), 12.0f, CRGBA(255, 255, 255, 255), 180);
            //CSprite2d::DrawCircleAtNearClip(CVector2D(sx, sy), 10.0f, CRGBA(20, 20, 20, 255), 180);
        }
     
    }
};


class CoronaData {
public:
    int type = 0;
    eCoronaFlareType flareType = eCoronaFlareType::FLARETYPE_NONE;

    int direction = 1;

    int numberOfSubCoronas = 2;

    unsigned char red = 255;
    unsigned char green = 0;
    unsigned char blue = 0;
    unsigned char alpha = 255;

    unsigned char sec_red = 0;
    unsigned char sec_green = 0;
    unsigned char sec_blue = 255;
    unsigned char sec_alpha = 255;

    unsigned char bigFlash_alpha = 20;

    int initialPattern = 0;

    float radius = 0.2f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 1.0f;
    float nearClip = 0.8f;
    float farClip = 200.0f;
    float bigFlashRadius = 6.0f;

    bool useSecondaryColor = false;

    bool useBigFlash = false;

    float distanceBetweenCoronas = 0.3f;
};

class ModelConfig {
public:
    unsigned short modelIndex;

    int numberOfCoronas = 0;

    std::list<CoronaData*>Coronas;

    ModelConfig(unsigned short modelIndex) {
        this->modelIndex = modelIndex;
    }

    void CreateCorona() {
        CoronaData* coronaData = new CoronaData();
        Coronas.push_back(coronaData);
        numberOfCoronas++;
    }

    void RemoveCorona(CoronaData* corona) {
        std::list<CoronaData*>::iterator it;
        for (it = Coronas.begin(); it != Coronas.end(); ++it) {
            if ((*it) == corona) {
                Coronas.erase(it);
                numberOfCoronas--;
                return;
            }
        }
    }

    CoronaData* GetCorona(int id) {
        int i = 0;
        std::list<CoronaData*>::iterator it;
        for (it = Coronas.begin(); it != Coronas.end(); ++it) {
            if (id == i) {
                return (*it);
            }
            i++;
        }
    }
};

class VehicleController {
public:
    CVehicle* vehicle;

    std::map<int, int>cData_pattern;
    std::map<int, int>cData_step;
    std::map<int, int>cData_lastStepChange;
    std::map<int, int>cData_lastPatternChange;


    bool active = false;

    bool setupPattern = true;


    void SetupPatterns(ModelConfig* modelCfg) {
        if (setupPattern) {
            setupPattern = false;

            for (int corona_i = 0; corona_i < modelCfg->numberOfCoronas; corona_i++)
            {
                CoronaData* corona = modelCfg->GetCorona(corona_i);

                cData_pattern[corona_i] = corona->initialPattern;
                cData_step[corona_i] = 0;
                cData_lastPatternChange[corona_i] = CTimer::m_snTimeInMilliseconds;
                cData_lastStepChange[corona_i] = CTimer::m_snTimeInMilliseconds;
            }
        }
    }

    VehicleController(CVehicle* vehicle) {
        this->vehicle = vehicle;
    }
};


// GV2

std::list<VehicleController*> vehicleControllers;
std::list<Input*> coronaButtons;
std::list<std::pair<unsigned short, ModelConfig*>>modelsConfig;
std::list<Input*> ListInputs;
std::list<Input*> ListInputs_coronaSlots;

Input* nextPage;
Input* prevPage;
//1
Input* cfgPage_numCoronas;
Input* cfgPage_removeCorona;
Input* cfgPage_dir;
Input* cfgPage_distCoronas;
Input* cfgPage_radius;
Input* cfgPage_type;
Input* cfgPage_initialPattern;
//2
Input* cfgPage_posX;
Input* cfgPage_posY;
Input* cfgPage_posZ;
Input* cfgPage_nearClip;
//3
Input* cfgPage_colorM_red;
Input* cfgPage_colorM_green;
Input* cfgPage_colorM_blue;
Input* cfgPage_colorM_alpha;
//4
Input* cfgPage_useSecColor;
Input* cfgPage_colorSec_red;
Input* cfgPage_colorSec_green;
Input* cfgPage_colorSec_blue;
Input* cfgPage_colorSec_alpha;
//5
Input* cfgPage_useBigFlash;
Input* cfgPage_bigFlashRadius;
Input* cfgPage_bigFlashAlpha;

Input* editField_editingInput;
CoronaData* menu_editingCoronaClass;

bool floatEditor_enabled = false;
bool floatEditor_movingView = false;
Input* floatEditor_editingInput;

// Functions 2
static void SaveConfigData() {
    std::fstream cfgFile;

    char path[256];
    sprintf(path, "%sNSiren.SA.data", paths::GetPluginDirPathA());
    cfgFile.open(path, std::fstream::out | std::fstream::trunc);

    std::list< std::pair<unsigned short, ModelConfig*> >::iterator it;
    for (it = modelsConfig.begin(); it != modelsConfig.end(); ++it) {
        ModelConfig* modelCfg = (*it).second;

        for (int i = 0; i < modelCfg->numberOfCoronas; i++)
        {
            CoronaData* corona = modelCfg->GetCorona(i);
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 0 << ":" << corona->numberOfSubCoronas << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 1 << ":" << corona->direction << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 2 << ":" << corona->distanceBetweenCoronas << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 3 << ":" << corona->radius << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 4 << ":" << corona->type << "\n";

            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 5 << ":" << corona->x << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 6 << ":" << corona->y << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 7 << ":" << corona->z << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 8 << ":" << corona->nearClip << "\n";

            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 9 << ":" << (int)corona->red << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 10 << ":" << (int)corona->green << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 11 << ":" << (int)corona->blue << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 12 << ":" << (int)corona->alpha << "\n";

            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 13 << ":" << corona->useSecondaryColor << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 14 << ":" << (int)corona->sec_red << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 15 << ":" << (int)corona->sec_green << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 16 << ":" << (int)corona->sec_blue << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 17 << ":" << (int)corona->sec_alpha << "\n";
   
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 18 << ":" << corona->useBigFlash << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 19 << ":" << corona->bigFlashRadius << "\n";
            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 20 << ":" << (int)corona->bigFlash_alpha << "\n";

            cfgFile << modelCfg->modelIndex << ":" << i << ":" << 21 << ":" << corona->initialPattern << "\n";

        }
    }

    cfgFile.close();
}

static void SetCoronaSettings() {
    CoronaData* corona = menu_editingCoronaClass;

    cfgPage_numCoronas->SetSelection(corona->numberOfSubCoronas, 0, 5);
    cfgPage_dir->SetSelection(corona->direction, 0, 1);
    cfgPage_distCoronas->floatValue = corona->distanceBetweenCoronas;
    cfgPage_radius->floatValue = corona->radius;
    cfgPage_type->SetSelection(corona->type, 0, 1);
    cfgPage_initialPattern->SetSelection(corona->initialPattern, 0, 3);

    cfgPage_posX->floatValue = corona->x;
    cfgPage_posY->floatValue = corona->y;
    cfgPage_posZ->floatValue = corona->z;
    cfgPage_nearClip->floatValue = corona->nearClip;

    cfgPage_colorM_red->SetSelection(corona->red, 0, 255);
    cfgPage_colorM_green->SetSelection(corona->green, 0, 255);
    cfgPage_colorM_blue->SetSelection(corona->blue, 0, 255);
    cfgPage_colorM_alpha->SetSelection(corona->alpha, 0, 255);

    cfgPage_useSecColor->SetSelection(corona->useSecondaryColor, 0, 1);
    cfgPage_colorSec_red->SetSelection(corona->sec_red, 0, 255);
    cfgPage_colorSec_green->SetSelection(corona->sec_green, 0, 255);
    cfgPage_colorSec_blue->SetSelection(corona->sec_blue, 0, 255);
    cfgPage_colorSec_alpha->SetSelection(corona->sec_alpha, 0, 255);

    cfgPage_useBigFlash->SetSelection(corona->useBigFlash, 0, 1);
    cfgPage_bigFlashRadius->floatValue = corona->bigFlashRadius;
    cfgPage_bigFlashAlpha->SetSelection(corona->bigFlash_alpha, 0, 40);
}

static void ApplyCoronaSettings() {
    CoronaData* corona = menu_editingCoronaClass;

    corona->numberOfSubCoronas = cfgPage_numCoronas->selection_current;
    corona->direction = cfgPage_dir->selection_current;
    corona->distanceBetweenCoronas = cfgPage_distCoronas->floatValue;
    corona->radius = cfgPage_radius->floatValue;
    corona->type = cfgPage_type->selection_current;
    corona->initialPattern = cfgPage_initialPattern->selection_current;

    corona->x = cfgPage_posX->floatValue;
    corona->y = cfgPage_posY->floatValue;
    corona->z = cfgPage_posZ->floatValue;
    corona->nearClip = cfgPage_nearClip->floatValue;

    corona->red = cfgPage_colorM_red->selection_current;
    corona->green = cfgPage_colorM_green->selection_current;
    corona->blue = cfgPage_colorM_blue->selection_current;
    corona->alpha = cfgPage_colorM_alpha->selection_current;

    corona->useSecondaryColor = cfgPage_useSecColor->selection_current;
    corona->sec_red = cfgPage_colorSec_red->selection_current;
    corona->sec_green = cfgPage_colorSec_green->selection_current;
    corona->sec_blue = cfgPage_colorSec_blue->selection_current;
    corona->sec_alpha = cfgPage_colorSec_alpha->selection_current;

    corona->useBigFlash = cfgPage_useBigFlash->selection_current;
    corona->bigFlashRadius = cfgPage_bigFlashRadius->floatValue;
    corona->bigFlash_alpha = cfgPage_bigFlashAlpha->selection_current;
    //cfgPage_bigFlashAlpha
}

static void SetCoronaSlotsInputVisible(bool visible) {
    std::list<Input*>::iterator input_itt;
    for (input_itt = ListInputs_coronaSlots.begin(); input_itt != ListInputs_coronaSlots.end(); ++input_itt) {
        Input* input = (*input_itt);
        
        input->show = visible;
    }
}

static void DrawInputs() {
    std::list<Input*>::iterator input_itt;
    for (input_itt = ListInputs.begin(); input_itt != ListInputs.end(); ++input_itt) {
        Input* input = (*input_itt);
        if (input->show) {
            input->Draw();
        }
    }
}

static Input* CreateInput(int type, float x, float y, float w, float h) {
    Input* input = new Input(type, x, y, w, h);
    ListInputs.push_back(input);
    return input;
}

static bool GetModelConfig(unsigned short modelIndex, ModelConfig* &modelCfg) {
    std::list< std::pair<unsigned short, ModelConfig*> >::iterator it;
    for (it = modelsConfig.begin(); it != modelsConfig.end(); ++it) {
        if ((*it).first == modelIndex) {
            modelCfg = (*it).second;
            return true;
        }
    }
    return false;
}

static ModelConfig* CreateModelConfig(unsigned short modelIndex) {
    ModelConfig* modelCfg = new ModelConfig(modelIndex);
    modelsConfig.push_back(std::make_pair(modelIndex, modelCfg));
    return modelCfg;
}

static bool GetVehicleController(CVehicle* vehicle, VehicleController* &vController) {
    std::list<VehicleController*>::iterator it;
    for (it = vehicleControllers.begin(); it != vehicleControllers.end(); ++it) {
        if ((*it)->vehicle == vehicle) {
            vController = (*it);
            return true;
        }
    }
    return false;
}

static VehicleController* CreateVehicleController(CVehicle* vehicle) {
    VehicleController* vController = new VehicleController(vehicle);
    vehicleControllers.push_back(vController);
    return vController;
}

static void RemoveVehicleController(VehicleController* vController) {
    std::list<VehicleController*>::iterator it;
    for (it = vehicleControllers.begin(); it != vehicleControllers.end(); ++it) {
        if (*it == vController) {
            break;
        }
    }
    vehicleControllers.erase(it);
}

static void EditCorona(int coronaSlot, ModelConfig* modelCfg) {
    menu_onScreen = 1;
    menu_editingCorona = coronaSlot;
    SetCoronaSlotsInputVisible(false);
    nextPage->show = true;
    prevPage->show = true;

    menu_editingCoronaClass = modelCfg->GetCorona(coronaSlot);

    //all inputs
    SetCoronaSettings();

}

static void DisableLights() {

    //https://gtaforums.com/topic/757430-block-siren-lights-memory-address-for-it/

    //0A8C: write_memory 0x70026C size 4 value 0x90909090 virtual_protect 0
    plugin::patch::SetUInt(0x70026C, 0x90909090);

    //0A8C : write_memory 0x700270 size 1 value 0x90 virtual_protect 0
    plugin::patch::SetUChar(0x700270, 0x90);

    //0A8C : write_memory 0x700271 size 1 value 0x90 virtual_protect 0
    plugin::patch::SetUChar(0x700271, 0x90);

    //0A8C : write_memory 0x700261 size 4 value 0x90909090 virtual_protect 0
    plugin::patch::SetUInt(0x700261, 0x90909090);

    //0A8C : write_memory 0x700265 size 1 value 0x90 virtual_protect 0
    plugin::patch::SetUChar(0x700265, 0x90);

    //0A8C : write_memory 0x700266 size 1 value 0x90 virtual_protect 0
    plugin::patch::SetUChar(0x700266, 0x90);

    //0A8C : write_memory 0x700257 size 4 value 0x90909090 virtual_protect 0
    plugin::patch::SetUInt(0x700257, 0x90909090);

    //0A8C : write_memory 0x70025B size 1 value 0x90 virtual_protect 0
    plugin::patch::SetUChar(0x70025B, 0x90);

    //0A8C : write_memory 0x70025C size 1 value 0x90 virtual_protect 0
    plugin::patch::SetUChar(0x70025C, 0x90);

    //--

    //0@ = 0xC3F12C //CPointLight => RGB
    int pointLight = 0xC3F12C;

    //0A8C: write_memory 0@ size 4 value 0.0 virtual_protect 0 // R
    plugin::patch::SetUInt(pointLight, 0);

    //0@ += 4
    pointLight += 4;

    //0A8C: write_memory 0@ size 4 value 0.0 virtual_protect 0  // G
    plugin::patch::SetUInt(pointLight, 0);

    //0@ += 4
    pointLight += 4;

    //0A8C: write_memory 2@ size 4 value 0.0 virtual_protect 0 
    plugin::patch::SetUInt(pointLight, 0);

    //--

    //NOPs the function that draws the coronnas
    //0A8C: write_memory 0x6ABA60 size 4 value 0x90909090 virtual_protect 0
    plugin::patch::SetUInt(0x6ABA60, 0x90909090);

    //0A8C: write_memory 0x6ABA64 size 1 value 0x90 virtual_protect 0
    plugin::patch::SetUChar(0x6ABA64, 0x90);

    //--

    //NOPs the function that checks wether the siren was activated or not
    //0A8C: write_memory 0x6FFDFC size 1 value 0x90 virtual_protect 0
    plugin::patch::SetUChar(0x6FFDFC, 0x90);

    //0A8C: write_memory 0x6FFDFD size 1 value 0x90 virtual_protect 0
    plugin::patch::SetUChar(0x6FFDFD, 0x90);

    //0A8C: write_memory 0x6FFDFE size 1 value 0x90 virtual_protect 0
    plugin::patch::SetUChar(0x6FFDFE, 0x90);

    //--

    //NOPs the function that activates the shadow drawing under the vehicle
    //0A8C: write_memory 0x70802D size 4 value 0x90909090 virtual_protect 0
    //plugin::patch::SetUInt(0x70802D, 0x90909090);
}

static void ProccessHoldToAddInputs() {
    if (menu_showGUI) {
        std::list<Input*>::iterator input_itt;
        for (input_itt = ListInputs.begin(); input_itt != ListInputs.end(); ++input_itt) {
            Input* input = (*input_itt);
            if (input->show) {
                if (input->type == 1) {
                    if (input->holdToAdd) {
                        if (input->mouseOverL) {
                            input->selection_current--;
                        }
                        if (input->mouseOverR) {
                            input->selection_current++;
                        }

                        if (input->selection_current > input->selection_max) {
                            input->selection_current = input->selection_max;
                        }
                        if (input->selection_current < input->selection_min) {
                            input->selection_current = input->selection_min;
                        }

                        ApplyCoronaSettings();
                    }
                }
            }
        }

    }
    
}


void LoadConfigData() {

    char path[256];
    sprintf(path, "%sNSiren.SA.data", paths::GetPluginDirPathA());
    int file = CFileMgr::OpenFile(path, "r");

    if (file > 0) {
        char buf[256];
        while (CFileMgr::ReadLine(file, buf, 256)) {
            std::string str(reinterpret_cast<char*>(buf));

            int modelid;
            int siren;
            int cmd;
            std::string value;


            int n = 0;

            size_t pos = 0;
            std::string token;
            while ((pos = str.find(":")) != std::string::npos) {
                token = str.substr(0, pos);

                if (n == 0) { modelid = std::stoi(token); }
                if (n == 1) { siren = std::stoi(token); }
                if (n == 2) { cmd = std::stoi(token); }

                str.erase(0, pos + 1);
                n++;
            }
            value = str;

            //lg << modelid << ":" << siren << ":" << cmd << ":" << value << "\n";

            ModelConfig* modelCfg;
            if (!GetModelConfig(modelid, modelCfg))
            {
                CreateModelConfig(modelid);
                GetModelConfig(modelid, modelCfg);
            }


            if (modelCfg->numberOfCoronas <= siren) {
                modelCfg->CreateCorona();
            }

            CoronaData* corona = modelCfg->GetCorona(siren);

            if (cmd == 0) { corona->numberOfSubCoronas = std::stoi(value); }
            if (cmd == 1) { corona->direction = std::stoi(value); }
            if (cmd == 2) { corona->distanceBetweenCoronas = std::stof(value); }
            if (cmd == 3) { corona->radius = std::stof(value); }
            if (cmd == 4) { corona->type = std::stoi(value); }

            if (cmd == 5) { corona->x = std::stof(value); }
            if (cmd == 6) { corona->y = std::stof(value); }
            if (cmd == 7) { corona->z = std::stof(value); }
            if (cmd == 8) { corona->nearClip = std::stof(value); }

            if (cmd == 9) { corona->red = (unsigned char)std::stoi(value); }
            if (cmd == 10) { corona->green = (unsigned char)std::stoi(value); }
            if (cmd == 11) { corona->blue = (unsigned char)std::stoi(value); }
            if (cmd == 12) { corona->alpha = (unsigned char)std::stoi(value); }

            if (cmd == 13) { corona->useSecondaryColor = std::stoi(value); }
            if (cmd == 14) { corona->sec_red = (unsigned char)std::stoi(value); }
            if (cmd == 15) { corona->sec_green = (unsigned char)std::stoi(value); }
            if (cmd == 16) { corona->sec_blue = (unsigned char)std::stoi(value); }
            if (cmd == 17) { corona->sec_alpha = (unsigned char)std::stoi(value); }

            if (cmd == 18) { corona->useBigFlash = std::stoi(value); }
            if (cmd == 19) { corona->bigFlashRadius = std::stoi(value); }
            if (cmd == 20) { corona->bigFlash_alpha = (unsigned char)std::stoi(value); }

            if (cmd == 21) { corona->initialPattern = std::stoi(value); }
        }
    }

    
    
}


static void OnMouseDown() {

    if (menu_showGUI) {
        ModelConfig* modelCfg;
        if (GetModelConfig(menu_editingModelIndex, modelCfg))
        {
            if (floatEditor_enabled) {
                floatEditor_enabled = false;
                mousePos.x = SCREEN_COORD_CENTER_X;
                mousePos.y = SCREEN_COORD_CENTER_Y;
                return;
            }

            std::list<Input*>::iterator input_itt;
            for (input_itt = ListInputs.begin(); input_itt != ListInputs.end(); ++input_itt) {
                Input* input = (*input_itt);
                if (input->show) {
                    if (input->type == 2 && !floatEditor_enabled && input->mouseOver) {
                        floatEditor_enabled = true;
                        floatEditor_editingInput = input; 
                    }

                    if (input->type == 1) {
                        if (!input->holdToAdd) {
                            if (input->mouseOverL) {
                                input->selection_current--;
                            }
                            if (input->mouseOverR) {
                                input->selection_current++;
                            }
                        }

                        if (input->selection_current > input->selection_max) {
                            input->selection_current = input->selection_max;
                        }
                        if (input->selection_current < input->selection_min) {
                            input->selection_current = input->selection_min;
                        }

                        ApplyCoronaSettings();

                        std::list<VehicleController*>::iterator it;
                        for (it = vehicleControllers.begin(); it != vehicleControllers.end(); ++it) {
                            (*it)->setupPattern = true;
                        }
                    }
                    
                }
            }

            if (menu_onScreen == 0) {

                int coronaSlot = 0;
                for (std::list<Input*>::iterator input_cfg_itt = ListInputs_coronaSlots.begin(); input_cfg_itt != ListInputs_coronaSlots.end(); ++input_cfg_itt) {
                    Input* input = (*input_cfg_itt);

                    if (input->mouseOver) {
                        if (modelCfg->numberOfCoronas > coronaSlot) {
                            EditCorona(coronaSlot, modelCfg);
                        }
                        else if (modelCfg->numberOfCoronas == coronaSlot) {
                            modelCfg->CreateCorona();
                            
                            EditCorona(coronaSlot, modelCfg);

                            
                        }
                    }
                    coronaSlot++;
                }
            }
            
            if (menu_onScreen == 1)
            {
                if (prevPage->mouseOver) { menu_subPage--; }
                if (nextPage->mouseOver) { menu_subPage++; }

                if (menu_subPage < 0) { menu_subPage = 0; }

                if (menu_subPage > 4) { menu_subPage = 4; }

                if (menu_subPage == 0) {
                    if (cfgPage_removeCorona->mouseOver && cfgPage_removeCorona->show) {
                        modelCfg->RemoveCorona(menu_editingCoronaClass);
                        menu_onScreen = 0;
                        menu_subPage = 0;
                        SetCoronaSlotsInputVisible(true);
                        nextPage->show = false;
                        prevPage->show = false;
                    }
                }

            }
        
            
        }
        
    }
}

// NSiren

class NSiren {
public:
    NSiren() {
        char logPath[256];
        sprintf(logPath, "%sNSiren.SA.log", paths::GetPluginDirPathA());
        //lg.open(logPath, std::fstream::out | std::fstream::trunc);

        DisableLights();

        InitPatterns();

       

        for (int i = 0; i < 8; i++)
        {
            ListInputs_coronaSlots.push_back(CreateInput(0, i < 4 ? 35.0f : 225.0f, 240.0f + (i % 4) * 30, 180.0f, 20.0f));
        }

        prevPage = CreateInput(0, 25.0f, 395.0f, 120.0f, 20.0f);
        prevPage->SetText("Pagina anterior");

        nextPage = CreateInput(0, 195.0f, 395.0f, 120.0f, 20.0f);
        nextPage->SetText("Proxima pagina");
        
        //1
        cfgPage_numCoronas = CreateInput(1, 150.0f, 235.0f, 80.0f, 20.0f);
        cfgPage_numCoronas->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_numCoronas->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_numCoronas->bgColorDisabled = CRGBA(40, 40, 40, 255);

        cfgPage_removeCorona = CreateInput(0, 300.0f, 235.0f, 100.0f, 20.0f);
        cfgPage_removeCorona->SetText("Deletar corona");

        cfgPage_dir = CreateInput(1, 150.0f, 260.0f, 120.0f, 20.0f);
        cfgPage_dir->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_dir->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_dir->bgColorDisabled = CRGBA(40, 40, 40, 255);

        cfgPage_radius = CreateInput(2, 150.0f, 285.0f, 120.0f, 20.0f);
        cfgPage_radius->SetFloatEditorSettings("Tamanho", 0.0f, 1.0f);

        cfgPage_type = CreateInput(1, 150.0f, 310.0f, 120.0f, 20.0f);
        cfgPage_type->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_type->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_type->bgColorDisabled = CRGBA(40, 40, 40, 255);

        cfgPage_initialPattern = CreateInput(1, 150.0f, 335.0f, 120.0f, 20.0f);
        cfgPage_initialPattern->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_initialPattern->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_initialPattern->bgColorDisabled = CRGBA(40, 40, 40, 255);

        //2

        cfgPage_posX = CreateInput(2, 150.0f, 235.0f, 120.0f, 20.0f);
        cfgPage_posX->SetFloatEditorSettings("Posicao X (Lados)", -3.0f, 3.0f);

        cfgPage_distCoronas = CreateInput(2, 150.0f, 260.0f, 120.0f, 20.0f);
        cfgPage_distCoronas->SetFloatEditorSettings("Distancia entre coronas", 0.0f, 3.0f);

        cfgPage_posY = CreateInput(2, 150.0f, 285.0f, 120.0f, 20.0f);
        cfgPage_posY->SetFloatEditorSettings("Posicao Y (Frente/Tras)", -8.0f, 8.0f);

        cfgPage_posZ = CreateInput(2, 150.0f, 310.0f, 120.0f, 20.0f);
        cfgPage_posZ->SetFloatEditorSettings("Posicao Z (Altura)", -5.0f, 5.0f);

        cfgPage_nearClip = CreateInput(2, 150.0f, 335.0f, 120.0f, 20.0f);
        cfgPage_nearClip->SetFloatEditorSettings("Near Clip", 0.1f, 2.0f);

        //3

        cfgPage_colorM_red = CreateInput(1, 150.0f, 260.0f, 120.0f, 20.0f);
        cfgPage_colorM_red->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_colorM_red->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_colorM_red->bgColorDisabled = CRGBA(40, 40, 40, 255);
        cfgPage_colorM_red->SetAsHoldToAdd();

        cfgPage_colorM_green = CreateInput(1, 150.0f, 285.0f, 120.0f, 20.0f);
        cfgPage_colorM_green->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_colorM_green->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_colorM_green->bgColorDisabled = CRGBA(40, 40, 40, 255);
        cfgPage_colorM_green->SetAsHoldToAdd();

        cfgPage_colorM_blue = CreateInput(1, 150.0f, 310.0f, 120.0f, 20.0f);
        cfgPage_colorM_blue->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_colorM_blue->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_colorM_blue->bgColorDisabled = CRGBA(40, 40, 40, 255);
        cfgPage_colorM_blue->SetAsHoldToAdd();

        cfgPage_colorM_alpha = CreateInput(1, 150.0f, 335.0f, 120.0f, 20.0f);
        cfgPage_colorM_alpha->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_colorM_alpha->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_colorM_alpha->bgColorDisabled = CRGBA(40, 40, 40, 255);
        cfgPage_colorM_alpha->SetAsHoldToAdd();

        //4
        cfgPage_useSecColor = CreateInput(1, 150.0f, 235.0f, 150.0f, 20.0f);
        cfgPage_useSecColor->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_useSecColor->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_useSecColor->bgColorDisabled = CRGBA(40, 40, 40, 255);

        cfgPage_colorSec_red = CreateInput(1, 150.0f, 260.0f, 120.0f, 20.0f);
        cfgPage_colorSec_red->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_colorSec_red->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_colorSec_red->bgColorDisabled = CRGBA(40, 40, 40, 255);
        cfgPage_colorSec_red->SetAsHoldToAdd();

        cfgPage_colorSec_green = CreateInput(1, 150.0f, 285.0f, 120.0f, 20.0f);
        cfgPage_colorSec_green->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_colorSec_green->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_colorSec_green->bgColorDisabled = CRGBA(40, 40, 40, 255);
        cfgPage_colorSec_green->SetAsHoldToAdd();

        cfgPage_colorSec_blue = CreateInput(1, 150.0f, 310.0f, 120.0f, 20.0f);
        cfgPage_colorSec_blue->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_colorSec_blue->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_colorSec_blue->bgColorDisabled = CRGBA(40, 40, 40, 255);
        cfgPage_colorSec_blue->SetAsHoldToAdd();

        cfgPage_colorSec_alpha = CreateInput(1, 150.0f, 335.0f, 120.0f, 20.0f);
        cfgPage_colorSec_alpha->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_colorSec_alpha->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_colorSec_alpha->bgColorDisabled = CRGBA(40, 40, 40, 255);
        cfgPage_colorSec_alpha->SetAsHoldToAdd();

        //5
        cfgPage_useBigFlash = CreateInput(1, 150.0f, 235.0f, 150.0f, 20.0f);
        cfgPage_useBigFlash->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_useBigFlash->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_useBigFlash->bgColorDisabled = CRGBA(40, 40, 40, 255);

        cfgPage_bigFlashRadius = CreateInput(2, 150.0f, 260.0f, 120.0f, 20.0f);
        cfgPage_bigFlashRadius->SetFloatEditorSettings("Tamanho", 0.5f, 8.0f);

        cfgPage_bigFlashAlpha = CreateInput(1, 150.0f, 285.0f, 120.0f, 20.0f);
        cfgPage_bigFlashAlpha->bgColor = CRGBA(158, 158, 158, 255);
        cfgPage_bigFlashAlpha->bgColorHover = CRGBA(120, 120, 120, 255);
        cfgPage_bigFlashAlpha->bgColorDisabled = CRGBA(40, 40, 40, 255);
        cfgPage_bigFlashAlpha->SetAsHoldToAdd();

        static int key_lastToggleMenu = 0;

        Events::initRwEvent += []
        {
            int txd = CTxdStore::AddTxdSlot("mytxd");
            CTxdStore::LoadTxd(txd, "MODELS\\FRONTEN_PC.TXD");
            TxdDef* txdDef = CTxdStore::AddRef(txd);
            CTxdStore::PushCurrentTxd();
            CTxdStore::SetCurrentTxd(txd);
            mouseSprite.SetTexture("mouse", "mouseA");
            CTxdStore::PopCurrentTxd(); 
        };


        Events::initGameEvent += []
        {
            ///lg << "Reading config\n";

            LoadConfigData();

            //lg << "Config OK\n";
        };

        Events::processScriptsEvent.before += []
        {
        
            CPed* playa = FindPlayerPed();

            if (menu_showGUI)
            {
                if (KeyPressed(32)) {
                    if (!floatEditor_movingView) {
                        floatEditor_movingView = true;
                        Command<0x01B4, int, bool>(0, true);
                    }
                }
                else {
                    if (floatEditor_movingView) {
                        floatEditor_movingView = false;
                        Command<0x01B4, int, bool>(0, false);

                        mousePos.x = SCREEN_COORD_CENTER_X;
                        mousePos.y = SCREEN_COORD_CENTER_Y;
                        
                    }
                }

            }
            

            
            

            if (!floatEditor_movingView) {
                if (KeyPressed(1)) {
                    
                    ProccessHoldToAddInputs();

                    if (!bMouseDown) {
                        bMouseDown = true;
                        OnMouseDown();
                    }

                    
                }
                else {
                    if (bMouseDown) {
                        bMouseDown = false;
                    }
                }
            }
            

            if (KeyPressed(90) && KeyPressed(17) && CTimer::m_snTimeInMilliseconds - key_lastToggleMenu > 500)
            {
                key_lastToggleMenu = CTimer::m_snTimeInMilliseconds;
      
                CVehicle* vehicle = FindPlayerVehicle(0, false);

                if (vehicle > 0) {
                    if (!menu_showGUI) {
                        ModelConfig* modelCfg;
                        unsigned short modelIndex = vehicle->m_nModelIndex;

                        if (!GetModelConfig(modelIndex, modelCfg)) { CreateModelConfig(modelIndex); }

                        //ok
                        menu_showGUI = true;
                        Command<0x01B4, int, bool>(0, false);

                        menu_onScreen = 0;
                        menu_subPage = 0;
                        menu_editingModelIndex = modelIndex;
                        SetCoronaSlotsInputVisible(true);
                        nextPage->show = false;
                        prevPage->show = false;
                    }
                    else {
                        if (!floatEditor_enabled) {
                            menu_showGUI = false;
                            Command<0x01B4, int, bool>(0, true);
                            SaveConfigData();
                        }
                    }
                }
                else {
                    CMessages::AddMessageJumpQ("Voce precisa estar em um veiculo!", 1000, 0, false);
                }
 
             
            }

            if (menu_showGUI) {
                POINT position;
                if (GetCursorPos(&position)) {

                    if (floatEditor_enabled) {

                        if (!floatEditor_movingView) {
                            floatEditor_editingInput->floatValue -= (SCREEN_COORD_CENTER_X - position.x) / 500;
                            floatEditor_editingInput->floatValue = clampf(floatEditor_editingInput->floatValue, floatEditor_editingInput->floatMin, floatEditor_editingInput->floatMax);
                            
                            ApplyCoronaSettings();
                        }

                        
                    }

                    mousePos.x -= SCREEN_COORD_CENTER_X - position.x;
                    mousePos.y -= SCREEN_COORD_CENTER_Y - position.y;
                    mousePos.x = clampf(mousePos.x, 0, SCREEN_COORD_CENTER_X * 2);
                    mousePos.y = clampf(mousePos.y, 0, SCREEN_COORD_CENTER_Y * 2);

                }
            }

          

            
            
            for (CVehicle* vehicle : CPools::ms_pVehiclePool)
            {
                VehicleController* vController;
                if (!GetVehicleController(vehicle, vController)) {
                    vController = CreateVehicleController(vehicle);
                    //lg << reinterpret_cast<int>(vController->vehicle) << " created\n";
                }
                else {
                    vController->active = vehicle->m_nVehicleFlags.bSirenOrAlarm;
                }
            }

            for (VehicleController* vController : vehicleControllers) {
                bool found = false;

                for (CVehicle* v : CPools::ms_pVehiclePool)
                {
                    if (v == vController->vehicle) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    //lg << reinterpret_cast<int>(vController->vehicle) << " not fonud. Removed\n";
                    RemoveVehicleController(vController);
                }
            }
            
        };

        Events::vehicleRenderEvent.before += [](CVehicle* vehicle)
        {
 
            VehicleController* vController;
            if (GetVehicleController(vehicle, vController))
            {
                ModelConfig* modelCfg;
                if (GetModelConfig(vehicle->m_nModelIndex, modelCfg))
                {
                    if (modelCfg->numberOfCoronas == 0) { return; }

                    vController->SetupPatterns(modelCfg);

                    unsigned int light_id = reinterpret_cast<unsigned int>(vehicle);

                    for (int corona_i = 0; corona_i < modelCfg->numberOfCoronas; corona_i++)
                    {
                        bool left, middle, right;
                        int delay = 100;

                        GetPatternStep(vController->cData_pattern[corona_i], vController->cData_step[corona_i], left, middle, right, delay);

                        if (CTimer::m_snTimeInMilliseconds - vController->cData_lastStepChange[corona_i] >= delay) {
                            vController->cData_lastStepChange[corona_i] = CTimer::m_snTimeInMilliseconds;
                            vController->cData_step[corona_i]++;

                            if (vController->cData_step[corona_i] >= GetNumberOfSteps(vController->cData_pattern[corona_i])) {
                                
                                vController->cData_step[corona_i] = 0;

                                if (CTimer::m_snTimeInMilliseconds - vController->cData_lastPatternChange[corona_i] > 8000) {
                                    vController->cData_lastPatternChange[corona_i] = CTimer::m_snTimeInMilliseconds;

                                    vController->cData_pattern[corona_i]++;
                                    if (vController->cData_pattern[corona_i] >= ListPatterns.size()) {
                                        vController->cData_pattern[corona_i] = 0;
                                    }
                                }
                               
                            }
                        }
                        
                        CoronaData* corona = modelCfg->GetCorona(corona_i);

                        for (int subCorona_i = 0; subCorona_i < corona->numberOfSubCoronas; subCorona_i++)
                        {
                            float radius = corona->radius;
                            float posx = corona->x - (corona->numberOfSubCoronas - 1) * corona->distanceBetweenCoronas / 2 + (subCorona_i)*corona->distanceBetweenCoronas;

                            CVector offset = CVector(posx, corona->y, corona->z);

                            bool isLeftSide = (subCorona_i < floor(corona->numberOfSubCoronas / 2));
                            bool isMiddle = corona->numberOfSubCoronas == 1;
                            bool isRightSide = (subCorona_i >= floor(corona->numberOfSubCoronas / 2) + (corona->numberOfSubCoronas % 2 == 0 ? 0 : 1));

                            bool showCorona = false;
                            bool registerCorona = true;

                            if (left == 1 && isLeftSide) { showCorona = true; }
                            if (right == 1 && isRightSide) { showCorona = true; }
                            if (middle == 1 && isMiddle) { showCorona = true; }

                            if (!vController->active) {
                                showCorona = false;
                            }

                            if (menu_showGUI) {
                                showCorona = true;
                                registerCorona = true;
                            }

                            float bigFlashRadius = corona->bigFlashRadius;

                            //dir
                            CVector pos = CModelInfo::ms_modelInfoPtrs[vehicle->m_nModelIndex]->m_pColModel->m_boundBox.m_vecMin;
                            CVector vehiclePos = vehicle->TransformFromObjectSpace(CVector(0.0f, 0.0f, 0.0f));
                            CVector coronaPos = vehicle->TransformFromObjectSpace(offset);

                            float dir = (float)GetAngleBetweenVectors(vehiclePos, coronaPos, TheCamera.GetPosition());
                            if (isnan(dir)) {
                                dir = 0.01f;
                            }
                            const float start_fadeout = 1.00;
                            const float end_fadeout = 1.60;

                            if ((corona->y >= 0.1f || corona->y <= -0.1f) && corona->direction != 1) {
                                if (dir >= start_fadeout && dir <= end_fadeout) {
                                    float r = (dir - start_fadeout) / (end_fadeout - start_fadeout);

                                    radius *= (1.0f - r);
                                    bigFlashRadius *= (1.0f - r);
                                }

                                if (dir >= end_fadeout && corona->direction == 1) {
                                    registerCorona = false;
                                }

                                if (corona->direction == 0 && dir >= end_fadeout) {
                                    registerCorona = false;
                                }
                            }

                 
                            
                            //--

                            if (!showCorona) {
                                radius = 0.0f;
                                bigFlashRadius = 0.0f;
                            }

                            eCoronaType type = (corona->type == 0 ? eCoronaType::CORONATYPE_SHINYSTAR : eCoronaType::CORONATYPE_HEADLIGHTLINE);

                            bool useSecundaryColor = (corona->useSecondaryColor && isRightSide);

                            if (registerCorona) {
                                CCoronas::RegisterCorona(light_id, vehicle, (useSecundaryColor ? corona->sec_red : corona->red), (useSecundaryColor ? corona->sec_green : corona->green), (useSecundaryColor ? corona->sec_blue : corona->blue), (useSecundaryColor ? corona->sec_alpha : corona->alpha), offset, radius, corona->farClip, type, corona->flareType, false, false, 0, 0.0f, false, corona->nearClip, 0, 15.0f, false, false);
                            }

                            light_id++;

                            if (corona->useBigFlash && registerCorona) {
                                CCoronas::RegisterCorona(light_id, vehicle, (useSecundaryColor ? corona->sec_red : corona->red), (useSecundaryColor ? corona->sec_green : corona->green), (useSecundaryColor ? corona->sec_blue : corona->blue), corona->bigFlash_alpha, offset, bigFlashRadius, corona->farClip, type, eCoronaFlareType::FLARETYPE_NONE, false, false, 0, 0.0f, false, 1.5f, 0, 15.0f, false, false);
                            }
                        
                            light_id++;
                        }
                    }
                }
            }

        };


       
        Events::drawHudEvent.before += []
        {
            


            if (menu_showGUI) {

                ModelConfig* modelCfg;
                if (GetModelConfig(menu_editingModelIndex, modelCfg))
                {
          

                    CFont::SetOrientation(ALIGN_LEFT);
                    CFont::SetDropShadowPosition(1);
                    CFont::SetBackground(false, false);
                    CFont::SetScale(ScreenPosX(0.3), ScreenPosY(0.9));
                    CFont::SetFontStyle(FONT_SUBTITLES);
                    CFont::SetProportional(true);
                    CFont::SetColor(CRGBA(255, 255, 255, 255));

                    if (floatEditor_enabled) {
                        CSprite2d::DrawRect(CRect(ScreenPosX(0.0f), ScreenPosY(0.0f), ScreenPosX(800.0f), ScreenPosY(600.0f)), CRGBA(0, 0, 0, 60));


                        if (!floatEditor_movingView) {
                            CSprite2d::DrawRect(CRect(ScreenPosX(10.0f), ScreenPosY(230.0f), ScreenPosX(260.0f), ScreenPosY(340.0f)), CRGBA(0, 0, 0, 140));

                            //floatEditor_editingInput->floatValue


                            char text[256];

                            CFont::SetOrientation(ALIGN_CENTER);
                            CFont::PrintString(ScreenPosX(135.0f), ScreenPosY(245), floatEditor_editingInput->floatTitle);

                            CFont::SetScale(ScreenPosX(0.6), ScreenPosY(1.2));
                            sprintf(text, "%.2f", floatEditor_editingInput->floatValue);
                            CFont::PrintString(ScreenPosX(135.0f), ScreenPosY(265.0f), text);

                            CFont::SetColor(CRGBA(255, 160, 0, 255));
                            CFont::SetScale(ScreenPosX(0.4), ScreenPosY(0.85));

                            CFont::PrintString(ScreenPosX(135.0f), ScreenPosY(290.0f), "ESPACO: Move a camera livremente");
                            CFont::PrintString(ScreenPosX(135.0f), ScreenPosY(305.0f), "MOUSE1: Finaliza edicao");
                        }

                        return;
                    }
                    
                    if (floatEditor_movingView) { return;  }

                    CSprite2d::DrawRect(CRect(ScreenPosX(20.0f), ScreenPosY(200.0f), ScreenPosX(20.0f + 400.0f), ScreenPosY(200.0f + 220.0f)), CRGBA(0, 0, 0, 255));
                    CSprite2d::DrawRect(CRect(ScreenPosX(20.0f), ScreenPosY(200.0f), ScreenPosX(20.0f + 400.0f), ScreenPosY(200.0f + 30.0f)), CRGBA(22, 22, 22, 255));


                    

                    char text[256];
                    if (menu_onScreen == 1) {
                        sprintf(text, "NSiren - Editando [%i]", menu_editingCorona);

                        sprintf(text, "NSiren - Editando [%i]", menu_editingCorona);
                    }   
                    else {
                        sprintf(text, "NSiren - (%i)", modelCfg->modelIndex);
                        CFont::PrintString(ScreenPosX(23.0f), ScreenPosY(400.0f), "Feito por DaniloM1301");
                    }


                    CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(206.0f), text);

                    cfgPage_numCoronas->show = false;
                    cfgPage_removeCorona->show = false;
                    cfgPage_dir->show = false;
                    cfgPage_radius->show = false;
                    cfgPage_type->show = false;
                    cfgPage_initialPattern->show = false;

                    cfgPage_posX->show = false;
                    cfgPage_distCoronas->show = false;
                    cfgPage_posY->show = false;
                    cfgPage_posZ->show = false;
                    cfgPage_nearClip->show = false;
        
                    cfgPage_colorM_red->show = false;
                    cfgPage_colorM_green->show = false;
                    cfgPage_colorM_blue->show = false;
                    cfgPage_colorM_alpha->show = false;

                    cfgPage_useSecColor->show = false;
                    cfgPage_colorSec_red->show = false;
                    cfgPage_colorSec_green->show = false;
                    cfgPage_colorSec_blue->show = false;
                    cfgPage_colorSec_alpha->show = false;

                    cfgPage_bigFlashRadius->show = false;
                    cfgPage_useBigFlash->show = false;
                    cfgPage_bigFlashAlpha->show = false;
                    


                    if (menu_onScreen == 1) {
                        //170  //395
                        CFont::SetOrientation(ALIGN_CENTER);
                        sprintf(text, "%i / %i", menu_subPage + 1, 5);
                        CFont::PrintString(ScreenPosX(170.0f), ScreenPosY(396.0f), text);

                        CFont::SetOrientation(ALIGN_LEFT);
                        
                        CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(360.0f), " ( Segure ESPACO para mover a camera )");

                        if (menu_subPage == 0) {
                            //+25
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(235.0f), "Numero de coronas");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(260.0f), "Direcao");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(285.0f), "Tamanho");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(310.0f), "Tipo");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(335.0f), "Padrao das luzes");

                            cfgPage_numCoronas->show = true;
                            sprintf(cfgPage_numCoronas->stringText, "%i", cfgPage_numCoronas->selection_current);

                            cfgPage_removeCorona->show = true;

                            cfgPage_dir->show = true;
                            sprintf(cfgPage_dir->stringText, "%s", (cfgPage_dir->selection_current == 0 ? ("Frente ou tras") : ("Todas as direcoes")));

                            cfgPage_radius->show = true;

                            cfgPage_type->show = true;
                            sprintf(cfgPage_type->stringText, "%s", (cfgPage_type->selection_current == 0 ? ("Tipo 1") : ("Tipo 2")));

                            cfgPage_initialPattern->show = true;
                            sprintf(cfgPage_initialPattern->stringText, "Padrao %i", cfgPage_initialPattern->selection_current + 1);
                        }

                        if (menu_subPage == 1) {
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(235.0f), "Posicao X (Lados)");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(260.0f), "Distancia entre coronas");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(285.0f), "Posicao Y (Frente/Tras)");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(310.0f), "Posicao Z (Altura)");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(335.0f), "Near Clip *");

                            cfgPage_posX->show = true;

                            cfgPage_distCoronas->show = true;

                            cfgPage_posY->show = true;

                            cfgPage_posZ->show = true;

                            cfgPage_nearClip->show = true;
                        }
                        
                        if (menu_subPage == 2) {
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(235.0f), "[ Cor ]");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(260.0f), "R - Vermelho");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(285.0f), "G - Verde");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(310.0f), "B - Azul");
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(335.0f), "A - Transparencia");

                            cfgPage_colorM_red->show = true;
                            sprintf(cfgPage_colorM_red->stringText, "%i", cfgPage_colorM_red->selection_current);

                            cfgPage_colorM_green->show = true;
                            sprintf(cfgPage_colorM_green->stringText, "%i", cfgPage_colorM_green->selection_current);

                            cfgPage_colorM_blue->show = true;
                            sprintf(cfgPage_colorM_blue->stringText, "%i", cfgPage_colorM_blue->selection_current);

                            cfgPage_colorM_alpha->show = true;
                            sprintf(cfgPage_colorM_alpha->stringText, "%i", cfgPage_colorM_alpha->selection_current);

                            CSprite2d::DrawRect(CRect(ScreenPosX(354.0), ScreenPosY(286.0), ScreenPosX(403.0f), ScreenPosY(335.0f)), CRGBA(255, 255, 255, 255));
                            CSprite2d::DrawRect(CRect(ScreenPosX(356.0), ScreenPosY(288.0), ScreenPosX(401.0f), ScreenPosY(333.0f)), CRGBA(cfgPage_colorM_red->selection_current, cfgPage_colorM_green->selection_current, cfgPage_colorM_blue->selection_current, cfgPage_colorM_alpha->selection_current));

                        }

                        if (menu_subPage == 3) {
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(235.0f), "[ Cor Secundaria ]");
                            

                            cfgPage_useSecColor->show = true;
                            sprintf(cfgPage_useSecColor->stringText, "%s", cfgPage_useSecColor->selection_current == 1 ? "Usar cor secundaria" : "Nao usar cor secundaria");

                            if (cfgPage_useSecColor->selection_current == 1) {
                                CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(260.0f), "R - Vermelho");
                                CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(285.0f), "G - Verde");
                                CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(310.0f), "B - Azul");
                                CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(335.0f), "A - Transparencia");

                                cfgPage_colorSec_red->show = true;
                                sprintf(cfgPage_colorSec_red->stringText, "%i", cfgPage_colorSec_red->selection_current);

                                cfgPage_colorSec_green->show = true;
                                sprintf(cfgPage_colorSec_green->stringText, "%i", cfgPage_colorSec_green->selection_current);

                                cfgPage_colorSec_blue->show = true;
                                sprintf(cfgPage_colorSec_blue->stringText, "%i", cfgPage_colorSec_blue->selection_current);

                                cfgPage_colorSec_alpha->show = true;
                                sprintf(cfgPage_colorSec_alpha->stringText, "%i", cfgPage_colorSec_alpha->selection_current);

                                CSprite2d::DrawRect(CRect(ScreenPosX(354.0), ScreenPosY(286.0), ScreenPosX(403.0f), ScreenPosY(335.0f)), CRGBA(255, 255, 255, 255));
                                CSprite2d::DrawRect(CRect(ScreenPosX(356.0), ScreenPosY(288.0), ScreenPosX(401.0f), ScreenPosY(333.0f)), CRGBA(cfgPage_colorSec_red->selection_current, cfgPage_colorSec_green->selection_current, cfgPage_colorSec_blue->selection_current, cfgPage_colorSec_alpha->selection_current));

                            }
                            

                            
                        }

                        if (menu_subPage == 4) {
                            CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(235.0f), "Usar flash maior");
                           

                            cfgPage_useBigFlash->show = true;
                            sprintf(cfgPage_useBigFlash->stringText, "%s", cfgPage_useBigFlash->selection_current == 1 ? "Ativado" : "Desativado");

                            if (cfgPage_useBigFlash->selection_current == 1) {
                                CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(260.0f), "Tamanho");
                                CFont::PrintString(ScreenPosX(25.0f), ScreenPosY(285.0f), "Intensidade");

                                cfgPage_bigFlashRadius->show = true;

                                cfgPage_bigFlashAlpha->show = true;
                                sprintf(cfgPage_bigFlashAlpha->stringText, "%i", cfgPage_bigFlashAlpha->selection_current);
                            }

                            

                        }
                    }

                    

                    if (menu_onScreen == 0) {
                        std::list<Input*>::iterator input_itt;
                        int input_i = 0;
                        for (input_itt = ListInputs_coronaSlots.begin(); input_itt != ListInputs_coronaSlots.end(); ++input_itt) {
                            Input* input = (*input_itt);

                            CoronaData* corona = modelCfg->GetCorona(input_i);
                          
                            input->useIcon = false;
                            if (input_i < modelCfg->numberOfCoronas) {
                                char inputText[256];
                                sprintf(inputText, "Editar [%i]", input_i);

                                input->SetBackgroundColor(CRGBA(122, 122, 122, 255));
                                input->SetBackgroundColorHover(CRGBA(73, 73, 73, 255));
                                input->SetText(inputText);
                                input->SetCoronaIcon(CRGBA(corona->red, corona->green, corona->blue, corona->alpha), corona->useSecondaryColor, CRGBA(corona->sec_red, corona->sec_green, corona->sec_blue, corona->sec_alpha));

                            }
                            else if (input_i == modelCfg->numberOfCoronas) {
                                input->SetBackgroundColor(CRGBA(226, 226, 226, 255));
                                input->SetBackgroundColorHover(CRGBA(153, 153, 153, 255));
                                input->SetText("Criar corona");
                            }
                            else {
                                input->SetBackgroundColor(CRGBA(30, 30, 30, 255));
                                input->SetBackgroundColorHover(CRGBA(30, 30, 30, 255));
                                input->SetText("Slot vazio");
                            }

                            input_i++;
                        }
                    }


                    DrawInputs();

                }
            }

        };

        Events::drawHudEvent.after += []
        {
            if (menu_showGUI && !floatEditor_enabled && !floatEditor_movingView) {
                mouseSprite.Draw(mousePos.x, mousePos.y, 30.0f, 30.0f, CRGBA(255, 255, 255, 255));
            }
        };
    }
} nSiren;

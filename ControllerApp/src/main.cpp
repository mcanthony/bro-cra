#include "ofMain.h"

#include "../../SharedData.h"
#include "ofxSharedMemory.h"
#include "ofxUI.h"

#define SOUND_DEVICE_ID 0



enum windowMoveMode
{
    WINDOW_MOVE_NONE = 0,
    WINDOW_MOVE_X,
    WINDOW_MOVE_Y,
    WINDOW_MOVE_RANDOM,
    WINDOW_MOVE_RANDOM_TWEEN
};



class ofApp : public ofBaseApp
{
    SharedData * mSharedData;
    ofxSharedMemory<SharedData *> mSharedMem;
    bool bConnected;
    
    ofxUISuperCanvas * gui1;
    ofxUISuperCanvas * gui2;
    
    ofSoundStream soundStream;
    float mWaveL[WAVE_SIZE];
    float mWaveR[WAVE_SIZE];
    float mSmoothedVol;
    float mSmoothBalance;
    float mGain;
    float mBangTh;
    bool mBang;
    float mWRindTh;
    string mWindowSize;
    windowMoveMode mWindowMode;
    
    Rectangle mCurrentWindowRect[NUM_VFXWIN];
    Rectangle mTargetWindowRect[NUM_VFXWIN];
    float mWindowMoveSpeed[NUM_VFXWIN];
    float mWindowTweenAccel;
    
    ofXml mXml;
    ofRectangle mDispray;
    
public:
    ofApp()
    {
        ofSetFrameRate(60);
        
        mWindowMode = WINDOW_MOVE_NONE;
        mWindowTweenAccel = 0.05;
        mWRindTh = 0.05;
        
        //----------
        // seup display shape
        //----------
        loadWindowShapeInfo();
        
        //----------
        // setup shared memory
        //----------
        mSharedData = new SharedData();
        mSharedData->level = 0;
        for (int i = 0; i < WAVE_SIZE; ++i) mSharedData->wave[i] = 0;
        mSharedData->mode = 0;
        mSharedData->bang_switch = false;
        for (int i = 0; i < NUM_VFXWIN; ++i)
        {
            for (int j = 0; j < NUM_TOGGLE; ++j) mSharedData->toggles[i][j] = false;
            mCurrentWindowRect[i].x = 0;
            mCurrentWindowRect[i].y = 0;
            mCurrentWindowRect[i].w = 320;
            mCurrentWindowRect[i].h = 240;
            mTargetWindowRect[i].x = 0;
            mTargetWindowRect[i].y = 0;
            mTargetWindowRect[i].w = 320;
            mTargetWindowRect[i].h = 240;
            mWindowMoveSpeed[i] = ofRandom(5, 20);
        }
        memcpy(mSharedData->rect, mCurrentWindowRect, sizeof(mCurrentWindowRect));
        mSharedData->kill = false;
        
        mSharedMem.setup(SHARED_DATA_KEY, sizeof(SharedData), true);
        bConnected = mSharedMem.connect();
        
        //----------
        // setup sound stream
        //----------
        soundStream.listDevices();
        soundStream.setDeviceID(SOUND_DEVICE_ID);
        mSmoothedVol = 0;
        mSmoothBalance = 0.93;
        mGain = 1.0;
        mBangTh = 0.6;
        mBang = false;
        soundStream.setup(this, 0, 2, 44100, WAVE_SIZE, 4);
        
        //----------
        // setup gui
        //----------
        gui1 = new ofxUISuperCanvas("PARAMETERS");
        gui1->addFPSSlider("FPS");
        gui1->addSlider("LEVEL(SMOOTHED)", 0, 1, &mSmoothedVol);
        gui1->addSlider("BANG_THRESHOLD", 0, 1, &mBangTh);
        gui1->addButton("BANG", &mBang);
        gui1->addSlider("SMOOTH_BALANCE", 0, 1, &mSmoothBalance);
        gui1->addSlider("GAIN", 0, 50, &mGain);
        gui1->addWaveform("CH.01(L)", mWaveL, WAVE_SIZE, -1, 1);
        gui1->addWaveform("CH.02(R)", mWaveR, WAVE_SIZE, -1, 1);
        gui1->addLabel("TOGGLES");
        gui1->addToggleMatrix("TOGGLE_MATRIX", NUM_VFXWIN, NUM_TOGGLE);
        gui1->autoSizeToFitWidgets();
        gui1->loadSettings("gui1.xml");
        ofAddListener(gui1->newGUIEvent, this, &ofApp::guiEvent);
        
        gui2 = new ofxUISuperCanvas("WINDOW_MODE");
        gui2->setPosition(gui1->getRect()->getWidth() + gui1->getRect()->getX() + 1, 0);
        gui2->addSlider("RANDOM_THRESHOLD", 0, 1, &mWRindTh);
        vector<string> wmitem;
        wmitem.push_back("NONE");
        wmitem.push_back("MOVE_X");
        wmitem.push_back("MOVE_Y");
        wmitem.push_back("RANDOM");
        wmitem.push_back("RANDOM_TWEEN");
        gui2->addDropDownList("WINDOW_MODE", wmitem)->setAllowMultiple(false);
        gui2->autoSizeToFitWidgets();
        gui2->loadSettings("gui2.xml");
        ofAddListener(gui2->newGUIEvent, this, &ofApp::guiEvent);
    }
    
    void update()
    {
        mSmoothedVol = ofClamp(mSmoothedVol, 0, 1);
        
        // if not connected, try reconnect every 5 seconds or so
        if (!bConnected)
        {
            if (ofGetFrameNum() % 300 == 0) bConnected = mSharedMem.connect();
        }
        else {
            //----------
            // update data
            //----------
            mSharedData->level = mSmoothedVol;
            memcpy(mSharedData->wave, mWaveL, sizeof(mWaveL));
            mSharedData->mode = 0;

            if (mSmoothedVol > mBangTh)
            {
                if (!mBang)
                {
                    sendBang();
                    mBang = true;
                }
            }
            else mBang = false;
            
            updateWindowPosition();
            
            mSharedMem.setData(mSharedData);
        }
    }
    
    void draw()
    {
        ofBackground(80);
        
        stringstream s;
        s << "set display shape: " << mDispray;
        ofSetColor(255);
        ofDrawBitmapString(s.str(), 20, ofGetHeight() - 10);
        
        // mouse
        ofSetColor(0, 255, 0);
        ofFill();
        ofCircle(mouseX, mouseY, 5);
    }
    
    void sendBang()
    {
        mSharedData->bang_switch = !mSharedData->bang_switch;
    }
    
    void sendKill()
    {
        mSharedData->kill = true;
    }
    
    void startVfxWindows(int n)
    {
        mSharedData->kill = false;
        for (int i = 0; i < n; ++i)
        {
            // TODO: to ritch
            string cmd = "open -n ../../../../../VFXWindowApp/bin/VFXWindowApp.app --args " + ofToString(i) + " &";
            system(cmd.c_str());
        }
    }
    
    void exit()
    {
        mSharedMem.close();
        gui1->saveSettings("gui1.xml");
        gui2->saveSettings("gui2.xml");
        delete mSharedData;
        delete gui1;
        delete gui2;
    }
    
    void saveWindowShapeInfo()
    {
        mXml.clear();
        mXml.addChild("DISPLAY");
        mXml.setTo("//DISPLAY");
        mXml.addValue("X", ofGetWindowRect().x);
        mXml.addValue("Y", ofGetWindowRect().y);
        mXml.addValue("W", ofGetWindowRect().getWidth());
        mXml.addValue("H", ofGetWindowRect().getHeight());
        mXml.save("ini.xml");
    }
    
    void loadWindowShapeInfo()
    {
        if (mXml.load("ini.xml"))
        {
            int x = 0;
            int y = 0;
            int w = 1280;
            int h = 800;
            if (mXml.setTo("//DISPLAY"))
            {
                if (mXml.exists("X")) x = mXml.getValue<int>("X");
                if (mXml.exists("Y")) y = mXml.getValue<int>("Y");
                if (mXml.exists("W")) w = mXml.getValue<int>("W");
                if (mXml.exists("H")) h = mXml.getValue<int>("H");
            }
            mDispray.set(x, y, w, h);
        }
    }
    
    void updateWindowPosition()
    {
        bool bTween = false;
        for (int i = 0; i < NUM_VFXWIN; ++i)
        {
            Rectangle * cr = &mCurrentWindowRect[i];
            Rectangle * tr = &mTargetWindowRect[i];
            
            switch (mWindowMode)
            {
                case WINDOW_MOVE_NONE:
                {
                    tr->x = mDispray.x;
                    tr->y = mDispray.y;
                    tr->w = 320;
                    tr->h = 240;
                }
                case WINDOW_MOVE_X:
                {
                    tr->x += mWindowMoveSpeed[i];
                    if (cr->x < mDispray.x)
                    {
                        tr->x = mDispray.x;
                        mWindowMoveSpeed[i] *= -1;
                    }
                    if (cr->x + cr->w > mDispray.getWidth())
                    {
                        tr->x = mDispray.getWidth() - cr->w;
                        mWindowMoveSpeed[i] *= -1;
                    }
                }
                    break;
                case WINDOW_MOVE_Y:
                {
                    tr->y += mWindowMoveSpeed[i];
                    if (cr->y < mDispray.y)
                    {
                        tr->y = mDispray.y;
                        mWindowMoveSpeed[i] *= -1;
                    }
                    if (cr->y + cr->h > mDispray.getHeight())
                    {
                        tr->y = mDispray.getHeight() - cr->h;
                        mWindowMoveSpeed[i] *= -1;
                    }
                }
                    break;
                case WINDOW_MOVE_RANDOM:
                {
                    float random = ofRandomuf();
                    if (random < mWRindTh)
                    {
                        tr->x = ofRandom(mDispray.x, (mDispray.getWidth() - cr->w) * 0.5);
                        tr->y = ofRandom(mDispray.y, (mDispray.getHeight() - cr->h) * 0.5);
                        tr->w = ofRandom(320, (mDispray.getWidth() - tr->x));
                        tr->h = ofRandom(240, (mDispray.getHeight() - tr->y));
                    }
                }
                    break;
                case WINDOW_MOVE_RANDOM_TWEEN:
                {
                    bTween = true;
                    float random = ofRandomuf();
                    if (random < mWRindTh)
                    {
                        tr->x = ofRandom(mDispray.x, mDispray.getWidth() - cr->w);
                        tr->y = ofRandom(mDispray.y, mDispray.getHeight() - cr->h);
                    }
                }
                    
                    break;
            }
            
            // tween
            if (bTween)
            {
                mCurrentWindowRect[i].x += (mTargetWindowRect[i].x - mCurrentWindowRect[i].x) * mWindowTweenAccel;
                mCurrentWindowRect[i].y += (mTargetWindowRect[i].y - mCurrentWindowRect[i].y) * mWindowTweenAccel;
            }
            else {
                mCurrentWindowRect[i].x = mTargetWindowRect[i].x;
                mCurrentWindowRect[i].y = mTargetWindowRect[i].y;
            }
            mCurrentWindowRect[i].w = mTargetWindowRect[i].w;
            mCurrentWindowRect[i].h = mTargetWindowRect[i].h;
        }
        
        memcpy(mSharedData->rect, mCurrentWindowRect, sizeof(mCurrentWindowRect));
    }
    
    void guiEvent(ofxUIEventArgs &e)
    {
        string name = e.widget->getName();
        int kind = e.widget->getKind();
        if (kind == 2)
        {
            ofxUIToggle * w = (ofxUIToggle *) e.widget;
            int vid = ofToInt(name.substr(name.size() - 2, 1));
            int tgl = ofToInt(name.substr(name.size() - 4, 1));
            mSharedData->toggles[vid][tgl] = w->getValue();
        }
        if (name == "WINDOW_MODE")
        {
            ofxUIDropDownList * dd = (ofxUIDropDownList *) e.widget;
            vector<int> &sel = dd->getSelectedIndeces();
            int tmp = 0;
            for (int i = 0; i < sel.size(); ++i) tmp = sel[i];
            mWindowMode = static_cast<windowMoveMode>(tmp);
            
            for (int i = 0; i < NUM_VFXWIN; ++i)
            {
                mWindowMoveSpeed[i] = ofRandom(5, 15);
            }
        }
    }
    
    void keyPressed(int key)
    {
        switch (key)
        {
            case 'f': ofToggleFullscreen(); break;
            case 's': saveWindowShapeInfo(); break;
            case 'l': loadWindowShapeInfo(); break;
            case 'b': sendBang(); break;
            case 'S': startVfxWindows(NUM_VFXWIN); break;
            case 'K': sendKill(); break;
        }
    }
    
    void audioIn(float * input, int bufferSize, int nChannels)
    {
        float curVol = 0.0;
        int numCounted = 0;
        for (int i = 0; i < bufferSize; i++)
        {
            mWaveL[i]	= input[i*2]    * 0.5 * mGain;
            mWaveR[i]	= input[i*2+1]  * 0.5 * mGain;
            curVol += mWaveL[i] * mWaveL[i];
            curVol += mWaveR[i] * mWaveR[i];
            numCounted += 2;
        }
        
        curVol /= (float)numCounted;
        curVol = sqrt( curVol );
        
        mSmoothedVol *= mSmoothBalance;
        mSmoothedVol += (1.0 - mSmoothBalance) * curVol;
    }
};



//========================================================================
int main( )
{
	ofSetupOpenGL(1024,768,OF_WINDOW);
	ofRunApp(new ofApp());
}

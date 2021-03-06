#pragma once

#include "common.h"
#include "ofxState.h"
#include "ofxEasingCam.h"
#include "ofxPostGlitch.h"

class BoxPerticle : public BaseState
{
    
//    ofImage mTexImg; // texture
    ofLight light;

    
    vector<ofBoxPrimitive> mBox;
    vector<ofColor> mCols;
    typedef vector<ofBoxPrimitive>::iterator box_it;
    
    ofxEasingCam rollCam;
    
    int mode;
    
    int mBoxNum;
    float mRotationSpeed;
    float mBoxSize;
    float mSpacing;
    float mCloudSize;
    bool mRandomCol;
    ofColor mCol;
    
    ofFbo fbo;
    ofxPostGlitch glitch;
    
public:
    GET_NAME
    
    BoxPerticle()
    {
        mBoxNum = 15;
        mRotationSpeed = 0.6;
        mBoxSize = 200;
        mSpacing = 5;
        mCloudSize = 100;
        mCol.set(180);
        mRandomCol = false;
        
        for (int i = 0; i < mBoxNum; i++) {
            mBox.push_back(ofBoxPrimitive());
        }
        
        fbo.allocate(640, 480, GL_RGB);
        assert(glitch.setup(&fbo, "glitch"));
    }
    
    void setup()
    {
        rollCam.setup();
        rollCam.setPos(0, 0, 800);
        rollCam.setCamSpeed(0.05);
        
//        light.setup();

        // setup boxs
        mCols.clear();
        for (vector<ofBoxPrimitive>::iterator box_it = mBox.begin(); box_it != mBox.end(); box_it++) {
            box_it->setResolution(1);
            box_it->setMode( OF_PRIMITIVE_TRIANGLE_STRIP );
            vector<ofMeshFace> triangles = box_it->getMesh().getUniqueFaces();
            box_it->getMesh().setFromTriangles( triangles, true );
            mCols.push_back(ofColor(255,255,255,255));
        }
        
        mode = 0;
    }
    
    void update()
    {
        mBoxSize = LEVEL * 600 + 100;
        
        float et = ofGetElapsedTimef();
        for (int i = 0; i < mBoxNum; i++) {
            float t = (et + i * mSpacing) * mRotationSpeed;
            ofVec3f pos(ofSignedNoise(t, 0, 0),
                        ofSignedNoise(0, t, 0),
                        ofSignedNoise(0, 0, t));
            float boxSize = mBoxSize * ofNoise(pos.x, pos.y, pos.z);
            
            pos *= mCloudSize;
            mBox[i].setPosition(pos);
            mBox[i].rotate(pos.x * 0.01, 1, 0, 0);
            mBox[i].rotate(pos.y * 0.01, 0, 1, 0);
            mBox[i].rotate(pos.z * 0.01, 0, 0, 1);
            mBox[i].set(boxSize);
        }
        
        rollCam.update();
    }
    
    void draw()
    {
        fbo.begin();
        const float w = fbo.getWidth();
        const float h = fbo.getHeight();
        
        if (TOGGLE[T0])
        {
            ofBackground(255);
            mCol.set(0, 0, 0);
        }
        else {
            ofBackground(0);
            mCol.set(255, 255, 255);
        }

        ofEnableLighting();
        light.setPosition(0, 0, 0);
        light.enable();
        ofEnableDepthTest();
        
        rollCam.begin(w, h);
        
        for (int i = 0; i < mBoxNum; i++) {
            if (TOGGLE[T1]) {
                ofSetColor(mCols[i]);
            } else {
                ofSetColor(mCol);
            }
            mBox[i].draw();
        }
        
        rollCam.end();
        
        light.disable();
        ofDisableLighting();
        ofDisableDepthTest();
        
        fbo.end();
        
        glitch.generateFx();
        ofSetColor(255, 255, 255);
        fbo.getTextureReference().draw(0, 0, WIDTH, HEIGHT);
    }

    void bang()
    {
        mode = (int)ofRandom(2);
        if (mode == 0) {
            rollCam.setRandomPos();
        }
        if (mode == 1) {
            rollCam.setRandomScale(0.3, 2);
        }
        if (TOGGLE[T1])
        {
            for (int i = 0; i < mBoxNum; i++) {
                ofColor c;
                c.setHsb(ofRandom(255), 210, 230);
                mCols[i].set(c);
            }
        }
        
        if (TOGGLE[T2])
        {
            switch((int)ofRandom(0, 3))
            {
                case 0: glitch.setFxTo(OFXPOSTGLITCH_CONVERGENCE, ofRandom(0.5, 1)); break;
                case 1: glitch.setFxTo(OFXPOSTGLITCH_CUTSLIDER, ofRandom(0.5, 1)); break;
                case 2: glitch.setFxTo(OFXPOSTGLITCH_TWIST, ofRandom(0.5, 1)); break;
            }
        }
    }
    

};


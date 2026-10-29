/*
 * File:   BasicTracker.h
 * Author: Joao F. Henriques, Joao Faro, Christian Bailer
 * Contact address: henriques@isr.uc.pt, joaopfaro@gmail.com, Christian.Bailer@dfki.de 
 * Instute of Systems and Robotics- University of COimbra / Department Augmented Vision DFKI 
 * 
 * This source code is provided for for research purposes only. For a commercial license or a different use case please contact us. 
 * You are not allowed to publish the unmodified sourcecode on your own e.g. on your webpage. Please refer to the official download page instead.
 * If you want to publish a modified/extended version e.g. because you wrote a publication with a modified version of the sourcecode you need our
 * permission (Please contact us for the permission).
 * 
 * We reserve the right to change the license of this sourcecode anytime to BSD, GPL or LGPL. 
 * By using the sourcecode you agree to possible restrictions and requirements of these three license models so that the license can be changed
 * anytime without you knowledge. 
 */



#pragma once

#include "gem5_vision.hpp"
// #include <string>

class Tracker
{
public:
    Tracker()  {}
   virtual  ~Tracker() { }

    virtual void init(const g5v::Rect &roi, g5v::Matrix image) = 0;
    virtual g5v::Rect update(g5v::Matrix image) = 0;


protected:
    g5v::Rect _roi;
};




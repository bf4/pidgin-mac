//
//  main.cpp
//  pidgin-macosx
//
//  Created by Albert Zeyer on 09.02.09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#include <Carbon/Carbon.h>
#include "TApplication.h"
#include "TWindow.h"

// Our custom application class
class CarbonApp : public TApplication
{
    public:
							
                            CarbonApp() {}
        virtual             ~CarbonApp() {}
        
    protected:
        virtual Boolean     HandleCommand( const HICommandExtended& inCommand );
};

// Our main window class
class MainWindow : public TWindow
{
    public:
                            MainWindow() : TWindow( CFSTR("MainWindow") ) {}
        virtual             ~MainWindow() {}
        
        static void         Create();
        
    protected:
        virtual Boolean     HandleCommand( const HICommandExtended& inCommand );
};

//--------------------------------------------------------------------------------------------
int main2(int argc, char* argv[])
{
    CarbonApp app;
    
    // Create a new window. A full-fledged application would do this from an AppleEvent handler
    // for kAEOpenApplication.
    MainWindow::Create();
    
    app.Run();
    return 0;
}

//--------------------------------------------------------------------------------------------
Boolean
CarbonApp::HandleCommand( const HICommandExtended& inCommand )
{
    switch ( inCommand.commandID )
    {
        case kHICommandNew:
            MainWindow::Create();
            return true;
            
        // Add your own command-handling cases here
        
        default:
            return false;
    }
}

//--------------------------------------------------------------------------------------------
void
MainWindow::Create()
{
    MainWindow* wind = new MainWindow();

    // Position new windows in a staggered arrangement on the main screen
    RepositionWindow( *wind, NULL, kWindowCascadeOnMainScreen );
    
    // The window was created hidden, so show it
    wind->Show();
}

//--------------------------------------------------------------------------------------------
Boolean
MainWindow::HandleCommand( const HICommandExtended& inCommand )
{
    switch ( inCommand.commandID )
    {
        // Add your own command-handling cases here
        
        default:
            return false;
    }
}

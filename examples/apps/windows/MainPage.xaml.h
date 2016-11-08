//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace Thread
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public ref class MainPage sealed
    {
    public:
        MainPage();

        void OnResuming();

        void BuildInterfaceList();

        void ConnectNetwork(Platform::Guid InterfaceGuid);
        void ShowInterfaceDetails(Platform::Guid InterfaceGuid);
        void DisconnectNetwork(Platform::Guid InterfaceGuid);

    protected:
        virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

    private:

        void OnLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnUnloaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

        void OnWindowSizeChanged(Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ args);
        void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ coreWindow, Windows::UI::Core::VisibilityChangedEventArgs^ args);

        UIElement^ CreateNewInterface(Platform::Guid InterfaceGuid); 

        bool _isVisible;
        bool _isFullScreen;

        Windows::Foundation::Size _windowSize;
        
        void *_apiInstance;
        #define ApiInstance ((otApiInstance*)_apiInstance)

        std::vector<void*> _devices;

        Platform::Guid _currentInterface;

    internal:
        static MainPage^ Current;

    };
}

import bb.cascades 1.3
import "settings"

Page {
    id: aboutPage

    signal closeRequested()

    actionBarVisibility: ChromeVisibility.Visible

    titleBar: TitleBar {
        title: qsTr("Thông Tin Ứng Dụng") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
        dismissAction: ActionItem {
            title: qsTr("Đóng") + Retranslate.onLanguageChanged
            onTriggered: {
                aboutPage.closeRequested();
            }
        }
    }

    actions: [
        ActionItem {
            title: qsTr("Cài Đặt") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_settings.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            shortcuts: [
                Shortcut { key: "S" }
            ]
            onTriggered: {
                aboutPage.closeRequested();
                app.openSettings();
            }
        }
    ]

    AboutContent {}
}

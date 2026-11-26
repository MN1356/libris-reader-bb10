import bb.cascades 1.3

Page {
    id: aboutSettingsPage

    signal backRequested()

    titleBar: TitleBar {
        title: qsTr("Thông Tin Ứng Dụng") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
    }

    AboutContent {}
}

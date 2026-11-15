import bb.cascades 1.3

Sheet {
    id: aboutSheet

    AboutPage {
        id: aboutPage
        onCloseRequested: {
            aboutSheet.close();
        }
    }
}

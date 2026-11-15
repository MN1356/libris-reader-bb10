/*
 * ToggleableListItem — a native BB10 settings row with a title, optional subtitle,
 * and a native ToggleButton on the right, with a standard Divider beneath.
 * Reused across settings pages following BBTelega pattern.
 */
import bb.cascades 1.3

Container {
    id: item

    property alias title: titleLabel.text
    property alias subtitle: subtitleLabel.text
    property bool checked: false
    property bool showDivider: true
    property bool switchEnabled: true

    signal toggled(bool checked)
    signal tapped()

    horizontalAlignment: HorizontalAlignment.Fill

    onCheckedChanged: {
        toggle.checked = checked;
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        leftPadding: 16
        rightPadding: 16
        topPadding: 12
        bottomPadding: 12

        layout: StackLayout {
            orientation: LayoutOrientation.LeftToRight
        }

        Container {
            verticalAlignment: VerticalAlignment.Center
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1.0
            }
            rightMargin: 12

            Label {
                id: titleLabel
                bottomMargin: 0
                textStyle.base: SystemDefaults.TextStyles.PrimaryText
                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
            }

            Label {
                id: subtitleLabel
                topMargin: 2
                visible: text !== ""
                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                multiline: true
            }

            gestureHandlers: [
                TapHandler {
                    onTapped: {
                        item.tapped();
                    }
                }
            ]
        }

        ToggleButton {
            id: toggle
            verticalAlignment: VerticalAlignment.Center
            checked: item.checked
            enabled: item.switchEnabled
            onCheckedChanged: {
                if (checked !== item.checked) {
                    item.toggled(checked);
                }
            }
        }
    }

    Divider {
        visible: item.showDivider
        topMargin: 0
        bottomMargin: 0
    }
}

/*
 * ToggleableListItem — full-bleed BB10 Cascades settings row with pressed feedback,
 * responsive dp() spacing, multiline labels, optional hairline divider, and native ToggleButton.
 */
import bb.cascades 1.3

Container {
    id: item

    property alias title: titleLabel.text
    property alias subtitle: subtitleLabel.text
    property bool checked: false
    property bool showDivider: true
    property bool switchEnabled: true
    property bool pressed: false

    signal toggled(bool checked)
    signal tapped()

    horizontalAlignment: HorizontalAlignment.Fill
    background: item.pressed ? Color.create(bookManager.uiActive) : Color.Transparent

    onCheckedChanged: {
        if (toggle.checked !== checked) {
            toggle.checked = checked;
        }
    }

    onTouch: {
        if (event.touchType === TouchType.Down) {
            item.pressed = true;
        } else if (event.touchType === TouchType.Up || event.touchType === TouchType.Cancel) {
            item.pressed = false;
        }
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        leftPadding: bookManager.dp(16)
        rightPadding: bookManager.dp(16)
        topPadding: bookManager.dp(12)
        bottomPadding: bookManager.dp(12)

        layout: StackLayout {
            orientation: LayoutOrientation.LeftToRight
        }

        Container {
            verticalAlignment: VerticalAlignment.Center
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1.0
            }
            rightMargin: bookManager.dp(12)

            Label {
                id: titleLabel
                bottomMargin: 0
                multiline: true
                textStyle.base: SystemDefaults.TextStyles.PrimaryText
                textStyle.color: Color.create(bookManager.uiInk)
            }

            Label {
                id: subtitleLabel
                topMargin: bookManager.dp(2)
                visible: text !== ""
                multiline: true
                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                textStyle.color: Color.create(bookManager.uiMuted)
            }

            gestureHandlers: [
                TapHandler {
                    onTapped: {
                        item.tapped();
                        if (item.switchEnabled) {
                            item.toggled(!item.checked);
                        }
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

    Container {
        visible: item.showDivider
        horizontalAlignment: HorizontalAlignment.Fill
        leftMargin: bookManager.dp(16)
        preferredHeight: 1
        background: Color.create(bookManager.uiHairline)
    }
}

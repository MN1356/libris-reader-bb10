/*
 * SliderStepperRow — numeric range control with responsive −/+ steppers and a native Slider.
 */
import bb.cascades 1.3

Container {
    id: row

    property string labelText: ""
    property string unit: " px"
    property int currentValue: 16
    property int minValue: 0
    property int maxValue: 60
    property int step: 2
    property bool showDivider: false
    property bool ready: false

    signal valueSelected(int newValue)

    horizontalAlignment: HorizontalAlignment.Fill
    leftPadding: bookManager.dp(16)
    rightPadding: bookManager.dp(16)
    topPadding: bookManager.dp(10)
    bottomPadding: bookManager.dp(6)

    onCurrentValueChanged: {
        if (ready && Math.round(rangeSlider.value) !== currentValue) {
            rangeSlider.value = currentValue;
        }
    }

    onCreationCompleted: {
        rangeSlider.fromValue = row.minValue;
        rangeSlider.toValue = row.maxValue;
        rangeSlider.value = row.currentValue;
        row.ready = true;
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

        Label {
            text: row.labelText
            verticalAlignment: VerticalAlignment.Center
            textStyle.base: SystemDefaults.TextStyles.BodyText
            textStyle.color: Color.create(bookManager.uiInk)
            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
        }

        Label {
            text: row.currentValue + row.unit
            verticalAlignment: VerticalAlignment.Center
            textStyle.base: SystemDefaults.TextStyles.BodyText
            textStyle.fontWeight: FontWeight.W500
            textStyle.color: Color.create(bookManager.uiAccent)
            rightMargin: bookManager.dp(12)
        }

        Button {
            text: "−"
            preferredWidth: bookManager.dp(44)
            preferredHeight: bookManager.dp(38)
            minHeight: bookManager.dp(38)
            verticalAlignment: VerticalAlignment.Center
            enabled: row.currentValue > row.minValue
            onClicked: {
                row.valueSelected(Math.max(row.minValue, row.currentValue - row.step));
            }
        }

        Button {
            text: "+"
            preferredWidth: bookManager.dp(44)
            preferredHeight: bookManager.dp(38)
            minHeight: bookManager.dp(38)
            leftMargin: bookManager.dp(6)
            verticalAlignment: VerticalAlignment.Center
            enabled: row.currentValue < row.maxValue
            onClicked: {
                row.valueSelected(Math.min(row.maxValue, row.currentValue + row.step));
            }
        }
    }

    Slider {
        id: rangeSlider
        horizontalAlignment: HorizontalAlignment.Fill
        topMargin: bookManager.dp(2)
        fromValue: row.minValue
        toValue: row.maxValue
        onImmediateValueChanged: {
            if (!row.ready) return;
            var rounded = Math.round(immediateValue);
            if (rounded !== row.currentValue) {
                row.valueSelected(rounded);
            }
        }
    }

    Container {
        visible: row.showDivider
        topMargin: bookManager.dp(8)
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: 1
        background: Color.create(bookManager.uiHairline)
    }
}

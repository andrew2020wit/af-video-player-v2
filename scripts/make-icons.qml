// Renders the application icon SVG to PNG files for scripts/make-icons.ps1.
// Arguments after "--": <svg file> <output folder> <name>:<size>:<scale> ...
// <scale> is the share of the PNG taken by the icon, the rest is a transparent margin.
import QtQuick

Window {
    id: window

    readonly property var args: Qt.application.arguments.slice(Qt.application.arguments.indexOf("--") + 1)
    property int jobIndex: 0
    property string jobName: ""
    property int jobSize: 1
    property real jobScale: 1

    function startJob() {
        const jobs = args.slice(2)

        if (jobIndex >= jobs.length) {
            Qt.quit()

            return
        }

        const parts = jobs[jobIndex].split(":")
        jobName = parts[0]
        jobSize = Number(parts[1])
        jobScale = Number(parts[2])
        Qt.callLater(grabJob)
    }

    function grabJob() {
        canvas.grabToImage(result => {
            if (!result.saveToFile(args[1] + "/" + jobName + ".png")) {
                console.error("Cannot save " + jobName)
                Qt.exit(1)

                return
            }

            jobIndex++
            startJob()
        }, Qt.size(jobSize, jobSize))
    }

    width: 1024
    height: 1024
    visible: true
    color: "transparent"

    Component.onCompleted: startJob()

    Item {
        id: canvas

        width: window.jobSize
        height: window.jobSize

        Image {
            readonly property int size: Math.round(window.jobSize * window.jobScale)

            anchors.centerIn: parent
            width: size
            height: size
            sourceSize: Qt.size(size, size)
            source: "file:///" + window.args[0]
            smooth: true
        }
    }
}

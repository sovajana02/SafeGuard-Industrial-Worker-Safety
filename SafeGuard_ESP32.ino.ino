from flask import Flask, jsonify
import serial
import threading
import sqlite3
from datetime import datetime

app = Flask(__name__)

SERIAL_PORT = "COM6"
BAUD_RATE = 9600

latest_data = {
    "distance": None,
    "mq2": None,
    "temperature": None,
    "humidity": None,
    "pcf_status": None,
    "time": None,
    "connected": False
}


# ---------------- DATABASE ----------------

def init_database():
    conn = sqlite3.connect("sensor_data.db")

    cursor = conn.cursor()

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS sensor_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            distance REAL,
            mq2 INTEGER,
            temperature REAL,
            humidity REAL,
            pcf_status INTEGER,
            time TEXT
        )
    """)

    conn.commit()
    conn.close()


# ---------------- SERIAL READER ----------------

def read_serial():

    global latest_data

    while True:

        try:

            print("Connecting to Arduino...")

            ser = serial.Serial(
                SERIAL_PORT,
                BAUD_RATE,
                timeout=1
            )

            latest_data["connected"] = True

            print("Connected to Arduino on", SERIAL_PORT)

            while True:

                line = ser.readline().decode(
                    "utf-8",
                    errors="ignore"
                ).strip()

                if not line:
                    continue

                if not line.startswith("DATA,"):
                    continue

                values = line.split(",")

                if len(values) != 6:
                    continue

                try:

                    distance = float(values[1])
                    mq2 = int(values[2])
                    temperature = float(values[3])
                    humidity = float(values[4])
                    pcf_status = int(values[5])

                    current_time = datetime.now().strftime(
                        "%Y-%m-%d %H:%M:%S"
                    )

                    latest_data = {
                        "distance": distance,
                        "mq2": mq2,
                        "temperature": temperature,
                        "humidity": humidity,
                        "pcf_status": pcf_status,
                        "time": current_time,
                        "connected": True
                    }

                    # Save to database

                    conn = sqlite3.connect(
                        "sensor_data.db"
                    )

                    cursor = conn.cursor()

                    cursor.execute("""
                        INSERT INTO sensor_data
                        (
                            distance,
                            mq2,
                            temperature,
                            humidity,
                            pcf_status,
                            time
                        )
                        VALUES (?, ?, ?, ?, ?, ?)
                    """, (
                        distance,
                        mq2,
                        temperature,
                        humidity,
                        pcf_status,
                        current_time
                    ))

                    conn.commit()
                    conn.close()

                    print(latest_data)

                except ValueError:

                    print(
                        "Invalid sensor data:",
                        line
                    )

        except serial.SerialException as error:

            latest_data["connected"] = False

            print(
                "Arduino connection error:",
                error
            )

            print(
                "Retrying in 3 seconds..."
            )

            import time
            time.sleep(3)


# ---------------- DASHBOARD ----------------

@app.route("/")
def dashboard():

    return """
<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width,
               initial-scale=1.0">

<title>
Hazardous Gas Detector
</title>

<style>

* {
    box-sizing: border-box;
}

body {

    margin: 0;

    font-family:
        Arial,
        Helvetica,
        sans-serif;

    background:
        #090d18;

    color: white;

    min-height: 100vh;

    padding-bottom: 30px;
}


.header {

    text-align: center;

    padding: 25px 10px 15px;

}

.header h1 {

    margin: 0;

    font-size: 28px;

}

.header p {

    color: #8f9bb3;

    margin-top: 8px;

}


/* MAIN */

.container {

    width: 95%;

    max-width: 900px;

    margin: auto;

}


/* CARDS */

.card {

    background:
        #171d31;

    border-radius: 20px;

    padding: 20px;

    margin-bottom: 18px;

    box-shadow:
        0 8px 25px
        rgba(0,0,0,0.25);

}


.card-title {

    color: #9ba6bd;

    font-size: 15px;

    font-weight: bold;

    letter-spacing: 1px;

    margin-bottom: 15px;

}


/* RADAR */

.radar {

    position: relative;

    width: 100%;

    max-width: 600px;

    height: 280px;

    margin: auto;

    overflow: hidden;

    background: #030711;

    border-radius: 15px;

}


/* radar circles */

.circle {

    position: absolute;

    left: 50%;

    bottom: -50%;

    transform:
        translateX(-50%);

    border:
        1px solid
        rgba(60,190,150,0.35);

    border-radius: 50%;

}


.c1 {

    width: 160px;
    height: 160px;

}

.c2 {

    width: 300px;
    height: 300px;

}

.c3 {

    width: 440px;
    height: 440px;

}

.c4 {

    width: 580px;
    height: 580px;

}


/* radar sweep */

.sweep {

    position: absolute;

    width: 50%;

    height: 2px;

    left: 50%;

    bottom: 0;

    transform-origin:
        left center;

    background:
        linear-gradient(
            90deg,
            #19ff8a,
            transparent
        );

    animation:
        sweep 3s linear infinite;

}


@keyframes sweep {

    from {

        transform:
            rotate(-170deg);

    }

    to {

        transform:
            rotate(10deg);

    }

}


.radar-center {

    position: absolute;

    width: 8px;

    height: 8px;

    background: #22ff91;

    border-radius: 50%;

    left: calc(50% - 4px);

    bottom: 0;

    box-shadow:
        0 0 15px
        #22ff91;

}


/* RADAR INFO */

.radar-info {

    display: flex;

    justify-content:
        space-between;

    align-items: center;

    margin-top: 15px;

    font-size: 18px;

}


.distance {

    font-size: 24px;

    font-weight: bold;

}


.status {

    padding:
        8px 15px;

    border-radius: 20px;

    background:
        #173b2c;

    color:
        #29ff99;

    font-weight: bold;

}


/* SENSOR GRID */

.sensor-grid {

    display: grid;

    grid-template-columns:
        repeat(2, 1fr);

    gap: 15px;

}


.sensor-card {

    background:
        #171d31;

    border-radius: 18px;

    padding: 20px;

}


.sensor-title {

    color: #929db3;

    font-size: 14px;

    font-weight: bold;

    letter-spacing: 1px;

}


.sensor-value {

    font-size: 30px;

    font-weight: bold;

    margin-top: 12px;

}


.sensor-unit {

    font-size: 16px;

    color: #aeb7c9;

}


/* GAS */

.gas-normal {

    color: #25ff91;

}


.gas-warning {

    color: #ffd84a;

}


.gas-danger {

    color: #ff5252;

}


/* SERVER */

.server-info {

    line-height: 1.8;

    color: #d7dbea;

}


.connection {

    color: #26ff98;

    font-weight: bold;

}


.disconnected {

    color: #ff5252;

    font-weight: bold;

}


/* UPDATE */

.update {

    text-align: center;

    color: #747f96;

    font-size: 13px;

    margin-top: 20px;

}


/* MOBILE */

@media(max-width:600px) {

    .header h1 {

        font-size: 23px;

    }

    .radar {

        height: 250px;

    }

    .sensor-value {

        font-size: 25px;

    }

    .sensor-grid {

        grid-template-columns:
            1fr 1fr;

    }

}

</style>

</head>


<body>


<div class="header">

<h1>
Hazardous Gas Detector
</h1>

<p>
Live Sensor Monitoring Dashboard
</p>

</div>


<div class="container">


<!-- RADAR -->

<div class="card">

<div class="card-title">
ULTRASONIC RADAR
</div>


<div class="radar">

<div class="circle c1"></div>

<div class="circle c2"></div>

<div class="circle c3"></div>

<div class="circle c4"></div>

<div class="sweep"></div>

<div class="radar-center"></div>

</div>


<div class="radar-info">

<div>
Distance:
<span
class="distance"
id="distance">
--
</span>
cm
</div>

<div
class="status"
id="distanceStatus">
CLEAR
</div>

</div>

</div>


<!-- SENSOR CARDS -->

<div class="sensor-grid">


<div class="sensor-card">

<div class="sensor-title">
TEMPERATURE
</div>

<div class="sensor-value">

<span id="temperature">
--
</span>

<span class="sensor-unit">
°C
</span>

</div>

</div>


<div class="sensor-card">

<div class="sensor-title">
HUMIDITY
</div>

<div class="sensor-value">

<span id="humidity">
--
</span>

<span class="sensor-unit">
%
</span>

</div>

</div>


<div class="sensor-card">

<div class="sensor-title">
MQ-2 GAS SENSOR
</div>

<div class="sensor-value">

<span id="mq2">
--
</span>

<span class="sensor-unit">
raw ADC
</span>

</div>

<div
id="gasStatus"
class="gas-normal">

STATUS:
NORMAL

</div>

</div>


<div class="sensor-card">

<div class="sensor-title">
BUZZER
</div>

<div class="sensor-value"
id="buzzer">

OFF

</div>

</div>


</div>


<!-- SERVER -->

<div class="card">

<div class="card-title">
SERVER STATUS
</div>

<div class="server-info">

Arduino:
<span
id="connection"
class="connection">
CONNECTED
</span>

<br>

Port:
COM6

<br>

Last update:
<span id="time">
--
</span>

<br>

PCF8574:
<span id="pcf">
--
</span>

</div>

</div>


<div class="update">

Live data via fetch('/api/data')
every 1 second — page never reloads

</div>


</div>


<script>


async function updateData() {

    try {

        const response =
            await fetch(
                "/api/data"
            );

        const data =
            await response.json();


        /* DISTANCE */

        if (
            data.distance !== null &&
            data.distance >= 0
        ) {

            document
                .getElementById(
                    "distance"
                )
                .innerText =
                    data.distance.toFixed(1);

        } else {

            document
                .getElementById(
                    "distance"
                )
                .innerText =
                    "--";

        }


        /* TEMPERATURE */

        if (
            data.temperature !== null &&
            data.temperature > -100
        ) {

            document
                .getElementById(
                    "temperature"
                )
                .innerText =
                    data.temperature.toFixed(1);

        } else {

            document
                .getElementById(
                    "temperature"
                )
                .innerText =
                    "--";

        }


        /* HUMIDITY */

        if (
            data.humidity !== null &&
            data.humidity >= 0
        ) {

            document
                .getElementById(
                    "humidity"
                )
                .innerText =
                    data.humidity.toFixed(1);

        } else {

            document
                .getElementById(
                    "humidity"
                )
                .innerText =
                    "--";

        }


        /* MQ2 */

        if (data.mq2 !== null) {

            document
                .getElementById(
                    "mq2"
                )
                .innerText =
                    data.mq2;

        }


        /* GAS STATUS */

        const gasStatus =
            document
                .getElementById(
                    "gasStatus"
                );


        if (data.mq2 >= 700) {

            gasStatus.innerText =
                "STATUS: DANGER";

            gasStatus.className =
                "gas-danger";

        }

        else if (data.mq2 >= 400) {

            gasStatus.innerText =
                "STATUS: WARNING";

            gasStatus.className =
                "gas-warning";

        }

        else {

            gasStatus.innerText =
                "STATUS: NORMAL";

            gasStatus.className =
                "gas-normal";

        }


        /* DISTANCE STATUS */

        const ds =
            document
                .getElementById(
                    "distanceStatus"
                );


        if (
            data.distance === null ||
            data.distance < 0
        ) {

            ds.innerText =
                "NO DATA";

        }

        else if (data.distance < 20) {

            ds.innerText =
                "DANGER";

        }

        else if (data.distance < 40) {

            ds.innerText =
                "WARNING";

        }

        else {

            ds.innerText =
                "CLEAR";

        }


        /* BUZZER */

        const buzzer =
            document
                .getElementById(
                    "buzzer"
                );


        if (
            data.distance !== null &&
            data.distance >= 0 &&
            data.distance < 40
        ) {

            buzzer.innerText =
                "ON";

        }

        else {

            buzzer.innerText =
                "OFF";

        }


        /* CONNECTION */

        const connection =
            document
                .getElementById(
                    "connection"
                );


        if (data.connected) {

            connection.innerText =
                "CONNECTED";

            connection.className =
                "connection";

        }

        else {

            connection.innerText =
                "DISCONNECTED";

            connection.className =
                "disconnected";

        }


        /* PCF */

        document
            .getElementById(
                "pcf"
            )
            .innerText =
                data.pcf_status == 1
                ? "OK"
                : "ERROR";


        /* TIME */

        document
            .getElementById(
                "time"
            )
            .innerText =
                data.time || "--";


    } catch(error) {

        console.log(
            "Update error:",
            error
        );

    }

}


/* FIRST UPDATE */

updateData();


/* LIVE UPDATE EVERY SECOND */

setInterval(
    updateData,
    1000
);

</script>


</body>

</html>
"""


# ---------------- API ----------------

@app.route("/api/data")
def api_data():

    return jsonify(latest_data)


# ---------------- START SERVER ----------------

if __name__ == "__main__":

    init_database()

    thread = threading.Thread(
        target=read_serial,
        daemon=True
    )

    thread.start()

    app.run(
        host="0.0.0.0",
        port=5000,
        debug=False
    )
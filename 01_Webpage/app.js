// Define the WebSocket connection
var webSocket = new WebSocket("ws://192.168.2.100:8000");

// WebSocket event handlers

// Call readAllData() after the WebSocket connection is established
webSocket.onopen = function(event) {
    console.log("Connection opened", event);
    document.getElementById('connection').textContent = "Connected";

    readUtilityData(["tv", "heater", "led_pwm", "lamp_floor", "lamp_ceil"]);
};

webSocket.onerror = function(error) {
    console.error("WebSocket Error", error);
    document.getElementById('connection').textContent = "No Connection";
};

webSocket.onmessage = function(message) {
    console.log("Message received: ", message.data);

    // Parse the JSON message
    var data = JSON.parse(message.data);
    var action = data.action;

    if(data.type === "CommandResponse") {
        var status = data.status;
        var message = data.message;
        if(status === "Success") {
            console.log("Command executed successfully");
        } else if(status === "Error"){
            console.error("Command %s failed: %s", action, message);
        } else{
            console.warn("Unknown status:", status);
        }
    } 
    else if(data.type === "DataResponse") {
        if(action === "read") {
            updateUIWithUtilityData(data.data);
        }
    }
    else {
        console.warn("Unknown response type or data update");
    }
};

// Event listeners for utility controls
document.addEventListener('DOMContentLoaded', function() {
    // Initialize Event Listeners
    attachEventListeners();

    // Weather and Time Update
    updateTime();
    updateDate();
    updateWeather();

    // Adjust Slider Size
    adjustSliderSize();
    $(window).resize(adjustSliderSize);

    // Start polling for alarm state and temperature
    setInterval(() => readUtilityData(["alarm", "temperature"]), 5000); // Poll every 5 seconds
});

// Attach event listeners to the UI controls
function attachEventListeners() {
    // Event listeners for utility controls
    document.getElementById('tv-toggle').addEventListener('click', toggleTV);
    document.getElementById('heater-toggle').addEventListener('click', toggleHeater);
    document.getElementById('lampFloor-toggle').addEventListener('click', toggleLampFloor);
    document.getElementById('lampCeil-toggle').addEventListener('click', toggleLampCeil);

    // Rear Lamp Sliders
    $("#lamp-plus-btn").click(function() {
        updateAndSendSliderValue("#lamp-slider", 1, "led_pwm");
    });
    $("#lamp-minus-btn").click(function() {
        updateAndSendSliderValue("#lamp-slider", -1, "led_pwm");
    });

    // Menu Button Click
    document.getElementById('menu-button').addEventListener('click', function() {
        var menu = document.getElementById('slide-menu');
        var menuButton = document.getElementById('menu-button');
        if (menu.style.right === '0px' || menu.style.right === '') {
            menu.style.right = '-250px'; // Hide menu
            menuButton.style.right = '1%'; // Reset button position
        } else {
            menu.style.right = '0px'; // Show menu
            menuButton.style.right = 'calc(5%)'; // Move button with menu
        }
    });

    // SUN Button clicked
    document.getElementById('sun-btn').addEventListener('click', function() {
        console.log("SUN button clicked");
    });

    // CLOUD Button clicked
    document.getElementById('cloud-btn').addEventListener('click', function() {
        console.log("CLOUD button clicked");
    });

    // RAIN Button clicked
    document.getElementById('rain-btn').addEventListener('click', function() {
        console.log("RAIN button clicked");
    });

    // SNOW Button clicked
    document.getElementById('snow-btn').addEventListener('click', function() {
        console.log("SNOW button clicked");
    });

    // GHOST Button clicked
    document.getElementById('ghost-btn').addEventListener('click', function() {
        console.log("GHOST button clicked");
    });

    // AUTO Button clicked
    document.getElementById('auto-btn').addEventListener('click', function() {
        console.log("AUTO button clicked");
    });
}

// Initialize the RoundSlider
$(document).ready(function() {
    // Initialize the RoundSlider for the lamp
    $("#lamp-slider").roundSlider({
        sliderType: "min-range",
        value: 25, // Example initial value
        change: function(event) {
            // This function is called when the slider value changes
            setSliderValue("led_pwm", event.value);
        }
    });
});

// Function to send a JSON command to the server
function sendCommand(commandObject) {
    var commandJSON = JSON.stringify(commandObject);
    webSocket.send(commandJSON);
    console.log("Command sent: ", commandJSON);
}


// Sends a JSON command to the server to read the specified utility data
// utility can be a string or an array of strings
function readUtilityData(utility) {
    var utilities = Array.isArray(utility) ? utility : [utility];
    sendCommand({ action: "read", utilities: utilities });
}

function toggleTV() {
    sendCommand({ action: "toggle", utility: "tv" });
    console.log("TV toggled");
}

function toggleHeater() {
    sendCommand({ action: "toggle", utility: "heater" });
    console.log("Heater toggled");
}

function toggleLampFloor() {
    sendCommand({ action: "toggle", utility: "lamp_floor" });
    console.log("Lamp 1 toggled");
}

function toggleLampCeil() {
    sendCommand({ action: "toggle", utility: "lamp_ceil" });
    console.log("Lamp 2 toggled");
}

function setSliderValue(utility, value) {
    sendCommand({ action: "write", utility: utility, value: value });
    console.log(`${utility} set to`, value);
}

// Function to update the slider value
function updateAndSendSliderValue(sliderId, change, utility) {
    var currentValue = $(sliderId).roundSlider("getValue");
    var newValue = currentValue + change;
    $(sliderId).roundSlider("setValue", newValue);
    setSliderValue(utility, newValue);
}

// ------------------ UI Update Functions ------------------

// Function to update the UI with utility data
function updateUIWithUtilityData(data) {
    //console.log("Updating UI with utility data:", data);

    // Update the TV state
    if(data.tv !== undefined) {
        updateTVState(data.tv);
    }

    // Update the alarm state
    if (data.alarm !== undefined) {
        updateAlarmState(data.alarm);
    }

    // Update the floor lamp state
    if(data.lamp_floor !== undefined) {
        updateLampFloorState(data.lamp_floor);
    }

    // Update the ceiling lamp state
    if(data.lamp_ceil !== undefined) {
        updateLampCeilState(data.lamp_ceil);
    }

    // Update the heater state
    if(data.heater !== undefined) {
        updateHeatherState(data.heater);
    }

    // Update the temperature
    if(data.temperature !== undefined) {
        var temperatureRounded = data.temperature.toFixed(2); // Rundet auf zwei Dezimalstellen
        document.getElementById('temp-status').textContent = temperatureRounded + " °C";
    } else {
        document.getElementById('temp-status').textContent = "Unknown Temp";
    }

    // Update the temperature
    if(data.led_pwm !== undefined) {
        $("#lamp-slider").roundSlider("setValue", data.led_pwm);
    }
}

// Function to update the TV state
function updateTVState(state) {
    // Logic to update TV's UI, e.g., toggling a button class
    var tvButton = document.getElementById('tv-toggle');
    if(state === 1) {
        tvButton.classList.add("button-toggled");
    } else {
        tvButton.classList.remove("button-toggled");
    }
}

// Function to update the Heater state
function updateHeatherState(state) {
    // Logic to update Heater's UI, e.g., toggling a button class
    var heatherButton = document.getElementById('heater-toggle');
    if(state === 1) {
        heatherButton.classList.add("button-toggled");
    } else {
        heatherButton.classList.remove("button-toggled");
    }
}

// Function to update the Alarm state
function updateAlarmState(state) {
    // Logic to update Alarm's UI, e.g., toggling a button class
    var alarmIcon = document.getElementById('alarm-icon');
    if (state === 1) {
        alarmIcon.classList.add('on');
    } else {
        alarmIcon.classList.remove('on');
    }
}

// Function to update the floor lamp state
function updateLampFloorState(state) {
    // Logic to update floor lamp's UI, e.g., toggling a button class
    var lampFloorButton = document.getElementById('lampFloor-toggle');
    if(state === 1) {
        lampFloorButton.classList.add("button-toggled");
    } else {
        lampFloorButton.classList.remove("button-toggled");
    }
}

// Function to update the ceiling lamp state
function updateLampCeilState(state) {
    // Logic to update ceiling lamp's UI, e.g., toggling a button class
    var lampCeilButton = document.getElementById('lampCeil-toggle');
    if(state === 1) {
        lampCeilButton.classList.add("button-toggled");
    } else {
        lampCeilButton.classList.remove("button-toggled");
    }
}

function adjustSliderSize() {
    var viewportWidth = $(window).width();
    
    // Calculate sizes based on viewport height or width
    var radius = viewportWidth * 0.06; 
    var width = viewportWidth * 0.005; 
    var handleSize = viewportWidth * 0.015;

    $(".roundslider").roundSlider({
    radius: radius,
    width: width,
    handleSize: handleSize,
    sliderType: "min-range",
    value: 25,
    tooltipFormat: "tooltipVal1"
    });
}

function updateTime() {
    var now = new Date();
    var formattedTime = now.getHours() + ':' + now.getMinutes().toString().padStart(2, '0');
    document.getElementById('time').textContent = formattedTime;
    setTimeout(updateTime, 1000); // Update the time every second
}

function updateDate() {
    var now = new Date();
    var formattedDate = now.getDate() + '.' + now.getMonth() + '.' + now.getFullYear();
    document.getElementById('date').textContent = formattedDate;
    setTimeout(updateTime, 1000); // Update the time every second
}

function updateWeather() {
    fetch('https://api.openweathermap.org/data/2.5/weather?lat=47.14&lon=7.25&appid=74678fe98ac424208ce3911f63e341ad')
        .then(response => response.json())
        .then(data => {
            var temperatureInKelvin = data.main.temp; // Get temperature in Kelvin
            var temperatureInCelsius = temperatureInKelvin - 273.15; // Convert to Celsius
            var iconCode = data.weather[0].icon; // Get the icon code from the API response
            var iconUrl = 'https://openweathermap.org/img/wn/' + iconCode + '.png'; // Construct icon URL

            // Set the weather text and icon
            document.getElementById('weather').innerHTML = 
                'Weather: ' + temperatureInCelsius.toFixed(0) + ' °C ' + 
                '<img src="' + iconUrl + '" alt="Weather Icon">';
        })
        .catch(error => console.error('Error fetching weather data:', error));
}

$(document).ready(function() {
    $('#button-container button').click(function() {
        $(this).toggleClass('button-toggled');
    });
});

function tooltipVal1(args) {
    return args.value + "%";
}
      
// Adjust the slider size on document ready and when the window is resized
$(document).ready(adjustSliderSize);
$(window).resize(adjustSliderSize);
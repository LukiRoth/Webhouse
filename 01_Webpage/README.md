# Webpage Directory

## Overview

This directory, `01_Webpage`, contains the front-end components of the Raspberry-Pi Webhouse project. It includes the HTML, CSS, and JavaScript files necessary to create the user interface through which users can interact with the Webhouse system.

## Contents

- **`webhouse.html`**: The main HTML file that users will load to interact with the Webhouse system. It provides the structural markup for the user interface.

- **`style.css`**: The Cascading Style Sheet (CSS) file that defines the visual style and layout of the web interface. It ensures that the user interface is visually appealing and user-friendly.

- **`app.js`**: The JavaScript file that contains the logic for handling user interactions, updating the user interface, and communicating with the server backend. It is essential for making the web interface dynamic and responsive.

## Usage

Before using make sure to adjust the IP address in the `app.js` file to match the IP address of the Raspberry-Pi running the server application.
After that to use the web interface, simply open the `webhouse.html` file in a modern web browser. Ensure that the server from the `02_Server` directory is running so that the web interface can communicate with it and control the Webhouse utilities.
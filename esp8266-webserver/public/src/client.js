"use strict";
class client {
    // elements must be
    //{
    //    background: background for color of door status
    //    status: status element
    //    open: open door command element
    //    close: close door command element
    //}
    constructor(elements) {

        this.doorStatusEnum = {
            Unknown: 0,
            IsOpen: 1,
            IsClosed: 2,
            Error: 3
        }

        this.openCloseModeEnum = {
            OpenClose: 0,
            Toggle: 1
        }

        this.commandEnum = {
            Open: "open",
            Close: "close",
            Toggle: "toggle"
        }

        this.doorStatus = this.doorStatusEnum.Unknown
        this.openCloseMode = this.openCloseModeEnum.Toggle

        this.elements = elements

        this.elements.open.addEventListener('click', this.openDoorCommand.bind(this))
        this.elements.close.addEventListener('click', this.closeDoorCommand.bind(this))
        this.elements.toggle.addEventListener('click', this.toggleDoorCommand.bind(this))        
        this.elements.mode.addEventListener('click', this.toggleMode.bind(this))        

        this.applyOpenCloseModeToDOM(this.openCloseMode)
    }

    start() {
        this.checkStatus()
        setInterval(this.checkStatus.bind(this), 3000)
    }

    checkStatus() {
        fetch("api/v1/status", { method: "GET", credentials: "include" })
        .then(res =>{
            if (res.status === 200) {
                res.text().then(r => {
                    if (r == "IsOpen") {
                        this.doorStatus = this.doorStatusEnum.IsOpen
                    } else if (r == "IsClosed") {
                        this.doorStatus = this.doorStatusEnum.IsClosed
                    }
                    this.applyDoorStatusToDOM(this.doorStatus)
                })
            } else {
                this.doorStatus = this.doorStatusEnum.Error
                this.applyDoorStatusToDOM(this.doorStatus)
            }
        })
        .catch(e => {
            console.log(e)
            this.doorStatus = this.doorStatusEnum.Error
            this.applyDoorStatusToDOM(this.doorStatus)
        })    
    }  

    applyDoorStatusToDOM(status) {
        switch(status) {
            case this.doorStatusEnum.Unknown:
                this.elements.status.innerText = "Door is unknown"
                this.elements.status.className = "unknown"
                break;
            
            case this.doorStatusEnum.IsOpen:
                this.elements.status.innerText = "Door is open"
                this.elements.status.className = "open"
                break;

            case this.doorStatusEnum.IsClosed:
                this.elements.status.innerText = "Door is closed"
                this.elements.status.className = "closed"
                break;

            case this.doorStatusEnum.Error:
                this.elements.status.innerText = "Error";
                this.elements.status.className = "error"
                break;
        }
    }
    
    sendDoorCommand (command) {
        fetch("api/v1/" + command, { method: "GET", credentials: "include" })
        .then(res => {
            if (res.status === 200) {
                //console.log(command + "  door command sent successfully")
                this.setCommandStatus(true)
            } else {
                //console.log(command + " door command not sent successfully")
                this.setCommandStatus(false)
            }
        })
        .catch(e => {
            this.setCommandStatus(false)
            console.log("Error sending " + command + " command")
            console.log(e)
        })
    }

    setCommandStatus(isSuccess) {
        if (isSuccess) {
            this.elements.commandStatus.className = "success"
            
        } else {
            this.elements.commandStatus.className = "error"
        }
        setTimeout(() => this.elements.commandStatus.className = "hidden", 3000)
    }

    openDoorCommand() {
        this.sendDoorCommand(this.commandEnum.Open)
    }

    closeDoorCommand() {
        this.sendDoorCommand(this.commandEnum.Close)
    }

    toggleDoorCommand() {
        if (confirm("Toggle door?")) {
            this.sendDoorCommand(this.commandEnum.Toggle)
        } else {
            // Do nothing
        }
    }

    toggleMode() {
        if (this.openCloseMode === this.openCloseModeEnum.OpenClose) {
            this.openCloseMode = this.openCloseModeEnum.Toggle
        } else {
            this.openCloseMode = this.openCloseModeEnum.OpenClose
        }
        this.applyOpenCloseModeToDOM(this.openCloseMode)
    }

    applyOpenCloseModeToDOM(mode) {
        switch(mode) {
            case this.openCloseModeEnum.OpenClose:
                this.elements.open.style.display = "flex"
                this.elements.close.style.display = "flex"
                this.elements.toggle.style.display = "none"
                break;

            case this.openCloseModeEnum.Toggle:
                this.elements.open.style.display = "none"
                this.elements.close.style.display = "none"
                this.elements.toggle.style.display = "flex"
                break;
        }
    }
}

let elements = {
    body: document.body,
    status: document.getElementById("status"),
    commandStatus: document.getElementById("commandStatus"),
    open: document.getElementById("openDoor"),
    close: document.getElementById("closeDoor"),
    toggle: document.getElementById("toggleDoor"),
    mode: document.getElementById("mode")
}
let cl = new client(elements)
cl.start();



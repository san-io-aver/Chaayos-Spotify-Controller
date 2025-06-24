from auth import sp
from flask import Flask, request

app = Flask(__name__)

preVol = 50

@app.route('/action', methods=['POST'])
def handleAction():
    global preVol
    data=request.get_json()
    action = data.get("action")

    if action == "pause":
        sp.pause_playback()
        return "Paused", 200
    elif action == "resume":
        sp.start_playback()
        return "resumed", 200
    elif action == "mute":
        current = sp.current_playback()
        if current and "device" in current:
            currentVol = current['device'].get('volume_percent',50)
            if currentVol>0:
                preVol = currentVol
        sp.volume(0)
        return "muted", 200
    elif action == "unmute":
        sp.volume(preVol if preVol>0 else 50) 
        return "unmuted", 200   
    
    else: return "NA", 400

app.run(host='0.0.0.0', port=5000)
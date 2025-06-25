import spotipy
from spotipy.oauth2 import SpotifyOAuth


CLIENT_ID = '######'
CLIENT_SECRET = '######'
REDIRECT_URI = '########'

SCOPE = 'user-read-playback-state user-modify-playback-state'

sp = spotipy.Spotify(auth_manager=SpotifyOAuth(
    client_id=CLIENT_ID,
    client_secret=CLIENT_SECRET,
    redirect_uri=REDIRECT_URI,
    scope=SCOPE
))
#functions:
# sp.pause_playback()
#sp.pause_playback()
#sp.next_track()
#sp.volume(vol_percent)

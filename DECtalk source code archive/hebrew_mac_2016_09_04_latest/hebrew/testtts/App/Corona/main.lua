-----------------------------------------------------------------------------------------
--
-- main.lua
--
-----------------------------------------------------------------------------------------

local dt_TTS = require( "dtTTS" )

-- local phTTS --storage for TTS handle
local bPollTTS = false
local iCurIndex = 0
local bPlayStatus = 0
local myText = display.newText( "Poll results:", 20, 100, native.systemFont, 10 )
myText:setReferencePoint( display.CenterLeftReferencePoint )
myText.x = 20
myText.y = 100

local function pollTTS()
  if ( bPollTTS == true ) then
    local iRes, bPlayStatus, iCurIndex = dt_TTS.DT_TextToSpeechGetStatusEx()
  
    local t = os.date( '*t' )  -- get table of current date and time
    --print( os.time( t ) )
    
    local sPlayStatus
    if bPlayStatus == 1 then
      sPlayStatus = "Playing"
    else
      sPlayStatus = "Stopped"
      ----bPollTTS = false --stop timer display
    end    
    -- Change the text
    
    myText.text = "Poll results: ".. t.hour ..":" .. t.min ..":" ..t.sec .." | " .. sPlayStatus .." | Idx: " .. iCurIndex   .. " | iResult: " .. iRes
    myText:setReferencePoint( display.CenterLeftReferencePoint )
    myText.x = 20

  end

end

timer.performWithDelay( 50, pollTTS,0 ) --infinite loop

-- show default status bar (iOS)
display.setStatusBar( display.DefaultStatusBar )

-- include Corona's "widget" library
local widget = require "widget"

-- event listeners for tab buttons:
local function onSetParams( event )
  print("Setting params")
  
  --Get the path to the base (R/O) folder with main.lua and other 
  local sPath = system.pathForFile( "dtalk_hb.dic" ) --specifies base directly
  sPath = string.sub(sPath,1,string.len(sPath) - 12)
  
  local iRes = dt_TTS.DT_TextToSpeechSetConfFileInfo(sPath)
  
  if not sPath then
    sPath = "no path"
  end
     
  print("sPath",sPath)
  myText.text = sPath
  myText:setReferencePoint( display.CenterLeftReferencePoint )
  myText.x = 20
    print ("iResult",iRes)  

end

local function onInitTTS( event )
  print("Initializing TTS")
  local iRes = dt_TTS.DT_TextToSpeechStartup()
  --opens TTS with the default audio device, DeviceOptions = SHARE_AUDIO_DEVICE and dwCallbackParameter = 0
  --returns handle to TTS which is stored in module-level var
  print ("iResult",iRes)  

end

local function onSpeak( event )
  print("Speaking")
  local newString="[:phon arpa on][V <,123>AH<192,123>-][YY<,123>O <192,123>-][T S <,155>EY<385,155>-][:index mark 16387][H <,155>AH<192,155>.][JH]"
  local sTextString = "[:punct pass][:phon arpa on] [:index start 250]  [:name paul] [:lang he] [:comma -40] [:period -140] [:volume att 80] [:rate 270][:mode reading on] [jh-]"
  sTextString = sTextString .. "[:index mark 16384][V <,123>AH<192,123>-][:index mark 16385][YY<,123>O <192,123>-][:index mark 16386][T S <,155>EY<385,155>-][:index mark 16387][H <,155>AH<192,155>-]"
  sTextString = sTextString .. "[:index mark 16388][EH<192,185>-] [:index mark 16389][EH<192,155>-] [:index mark 16390][EH<192,207>-][:index mark 16391][V <,185>EH<288,185>D <,185>-]"
  sTextString = sTextString .. "[:index mark 16392][JH<96,>-][:index mark 16393][K <,123>AX<72,123>-][:index mark 16394][L <,123>EY<192,123>-][:index mark 16395][CZ<,155>EH<385,155>-]"
  sTextString = sTextString .. "[:index mark 16396][S <,155>EH<385,155>F <,155>-][:index mark 16397][U <192,155>CZ<,155>-][:index mark 16398][L <,155>EY<192,155>-] [:index mark 16399][EY<192,92>-]"
  sTextString = sTextString .. " [:index mark 16400][EY<192,123>-][:index mark 16401][Z <,123>AH<192,123>-][:index mark 16402][H <,185>AH<288,185>V <,185>-][:index mark 16403][JH<144,>-]"
  sTextString = sTextString .. "[:index mark 16404][U <192,155>V <,155>-][:index mark 16405][G <,155>AH<192,155>-][:index mark 16406][D <,185>IY<192,185>-] [:index mark 16407][IY<192,138>M <,138>-]"
  sTextString = sTextString .. "[:index mark 16408][JH<288,>-][:index mark 16409][V <,155>AH<192,155>-][:index mark 16410][YY<,155>IY<192,155>-][:index mark 16411][T<,155>EY<192,155>-]"
  sTextString = sTextString .. " [:index mark 16412][EY<192,185>-] [:index mark 16413][EY<192,138>N <,138>-][:index mark 16414][JH<192,>-][:index mark 16415][L <,92>AX<72,92>-][:index mark 16416]"
  sTextString = sTextString .. "[R <,92>IY<192,92>V <,92>-][:index mark 16417][K <,138>AH<385,138>-][:index mark 16418][JH<385,>-][:index mark 16419][U <192,123>-]"
  sTextString = sTextString .. "[:index mark 16420][M <,155>IY<385,155>G <,155>-][:index mark 16421][DD<,155>AH<192,155>-][:index mark 16422][N <,185>O <192,185>-]"
  sTextString = sTextString .. " [:index mark 16423][O <192,138>T <,138>-][:index mark 16424][JH<288,>-][:index mark 16425][N <,116>AH<192,116>-][:index mark 16426][T <,138>AH<385,138>N <,138>-]"
  sTextString = sTextString .. "[:index mark 16427][L <,138>AX<48,138>-][:index mark 16428][AH<192,138>-][:index mark 16429][CH<,155>IY<192,155>-] [:index mark 16430][IY<192,185>-]"
  sTextString = sTextString .. "[:index mark 16431][H <,138>AH<192,138>-][:index mark 16432][JH<192,>-][:index mark 16433][U <192,138>L <,138>-][:index mark 16434][IY<192,138>-]"
  sTextString = sTextString .. "[:index mark 16435][MM<,123>AH<192,123>-] [:index mark 16436][AH<192,92>HH<,92>.][JH]"
  
  bPollTTS = true
  
  local iRes =  dt_TTS.DT_TextToSpeechSpeak(sTextString)
  
  print ("iResult",iRes)  
end

local function onReset( event )
  print("Resetting")
  
  local iRes = dt_TTS.DT_TextToSpeechReset()

  print ("iResult",iRes)  

end

local function onDeInitTTS( event )
  bPollTTS = false
  print("DeIniting")
  local iRes = dt_TTS.DT_TextToSpeechShutdown()
  print ("iResult",iRes)  
  
end

-- create a tabBar widget with five buttons at the bottom of the screen

-- table to setup buttons
local tabButtons = {
	{ label="SetParams", up="icon1.png", down="icon1-down.png", width = 32, height = 32, onPress=onSetParams },
	{ label="Init", up="icon1.png", down="icon1-down.png", width = 32, height = 32, onPress=onInitTTS },
	{ label="Speak", up="icon1.png", down="icon1-down.png", width = 32, height = 32, onPress=onSpeak },
	{ label="Reset", up="icon1.png", down="icon1-down.png", width = 32, height = 32, onPress=onReset },
	{ label="DeInit", up="icon1.png", down="icon1-down.png", width = 32, height = 32, onPress=onDeInitTTS },
	
}

-- create the actual tabBar widget
local tabBar = widget.newTabBar{
	top = display.contentHeight - 50,	-- 50 is default height for tabBar widget
	buttons = tabButtons
}


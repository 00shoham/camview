/* what are we zoomed into right now? */
var zoomCamera = "";
var zoomFile = "";
var desiredStepFile = "";
var zoomLive = 0

function LoadSingleSpecificImageDetailedOnly( camID, fileName )
  {
  var detailsID = document.getElementById( "photo-details" );
  if( detailsID )
    {
    var src = detailsID.src;
    if( src.includes( "cam="+camID+"\&" ) )
      {
      zoomFile = fileName;
      var url = "single-image?cam=" + camID + "\&file=" + fileName;
      detailsID.src = url;
      if( zoomLive )
        {
        detailsID.style.border = "3px solid #933";
        window.setTimeout(function(){ detailsID.style.border = "3px solid #fff"; }, 1000);
        }
      var labelID = document.getElementById( "label" );
      if( labelID )
        {
        var lString = fileName;
        lString = lString.replace( "image-", "" );
        lString = lString.replace( ".jpg", "" );
        lString = lString.replace( "_", "  " );
        labelID.innerHTML = "<p>" + camID + ": " + lString + "</p>";
        labelID.style.color = "#606";
        labelID.style.fontFamily = "sans-serif";
        labelID.style.fontWeight = "bold";
        labelID.style.fontSize = "30px";
        }
      }
    }
  }

function LoadSingleSpecificImage( camID, fileName )
  {
  var objID = document.getElementById( "img-" + camID );
  if( objID )
    {
    var url = "single-image?cam=" + camID
            + "\&file=" + fileName
            + "\&maxwidth=500";
    objID.src = url;
    objID.style.border = "3px solid #933";

    window.setTimeout(function(){ objID.style.border = "3px solid #fff"; }, 1000);
    }

  if( zoomLive || fileName === desiredStepFile )
    { /* don't refresh the big image if the user is busy doing prev/next navigation */
    /* are we zoomed into the current image?  reload that too then */
    LoadSingleSpecificImageDetailedOnly( camID, fileName );
    }
  }

function StateChangedInitialState( xhttp, camID )
  {
  if( xhttp.readyState == 4 && xhttp.status == 200 )
    {
    var tmpName = xhttp.responseText;
    var cameraParams = JSON.parse( tmpName );
    var objID = document.getElementById( "img-" + camID );
    if( 'latestFileAge' in cameraParams )
      {
      if( cameraParams.latestFileAge > 60*30 ) /* 30 minutes */
        {
        /* alert( "Camera "+camID+" has no recent image (bad)" ); */
        if( objID )
          {
          objID.style.opacity="0.5";
          objID.style.border = "3px solid #bbb";
          }
        }
      else
        {
        /* alert( "Camera "+camID+" has recent images (good)" ); */
        }
      }
    else
      {
      /* alert( "Can't tell if camera "+camID+" has recent images (very bad)" ); */
      if( objID )
        {
        objID.style.opacity="0.25";
        objID.style.border = "3px solid #bbb";
        }
      }
    }
  }

function InitialState( camID )
  {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() { StateChangedInitialState( xhttp, camID ); };
  var url = "single-image?cam="+camID+"\&info";
  xhttp.open( "GET", url, true );
  xhttp.send();
  }

function StateChangedGetLatestFilename( xhttp, camID )
  {
  if( xhttp.readyState == 4 && xhttp.status == 200 )
    {
    var tmpName = xhttp.responseText;
    var newName = tmpName.replace(/[ \t\r\n]/g,'');
    if( newName.includes('doctype') || newName.includes('html') )
      { /* not a filename */
      }
    else
      {
      var i = 0;
      for( i=0; i<nCams; i++ )
        {
        var cam = cameras[i];
        if( cam.name === camID )
          {
          if( ! (cam.imageFilename === newName) )
            {
            LoadSingleSpecificImage( camID, newName );
            cam.imageFilename = newName;
            }
          }
        }
      }
    }
  }

function GetLatestFilename( camID )
  {
  var i = 0;
  for( i=0; i < nCams; i++ )
    {
    var cam = cameras[i];
    if( cam.name == camID )
      {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() { StateChangedGetLatestFilename( xhttp, camID ); };
      var url = "single-image?cam="+camID+"\&nameonly";
      xhttp.open( "GET", url, true );
      xhttp.send();
      }
    }
  }

function InitialStateAllCameras()
  {
  var i = 0;
  for( i=0; i < nCams; i++ )
    {
    var cam = cameras[i];
    InitialState( cam.name );
    }
  }

function GetLatestAllCameras()
  {
  var i = 0;
  for( i=0; i < nCams; i++ )
    {
    var cam = cameras[i];
    GetLatestFilename( cam.name );
    }
  }

function ReloadImage( camID )
  {
  var objID = document.getElementById( "details" );
  if( objID )
    {
    details.style.display = "block";
    }

  objID = document.getElementById( "img-" + camID );
  if( objID )
    {
    var url = "single-image?cam=" + camID + "\&time="
              + new Date().getTime()
              + "\&maxwidth=500";
    objID.src = url;
    }
  GetLatestFilename( camID );
  }

function AddNavigationToDetails()
  {
  var divID = document.getElementById( "details" );
  if( ! divID )
    {
    console.log( "Cannot find ID 'details'" );
    return;
    }
  var rect = divID.getBoundingClientRect();

  if( !rect || rect.top<5 || rect.left<5 || rect.width<100 || rect.height<100 )
    {
    /* something went horribly wrong - the image isn't as large
       or as centered as we thought! */
    console.log( "Zoomed in, but rect is funny" );
    if( !rect ) console.log( "--> failed on getBoundingClientRect" );
    else if( rect.top<5 ) console.log( "--> rect.top = "+rect.top+" (<5)" );
    else if( rect.left<5 ) console.log( "--> rect.left = "+rect.left+" (<5)" );
    else if( rect.width<100 ) console.log( "--> rect.width = "+rect.width+" (<100)" );
    else if( rect.height<100 ) console.log( "--> rect.height = "+rect.height+" (<100)" );
    }
  else
    {
    var prevID = document.getElementById( "prev" );
    if( prevID )
      {
      var t = rect.top + (rect.bottom-rect.top)/2 - 10;
      var l = rect.left + 5;
      prevID.style.display = "block";
      prevID.style.top = t;
      prevID.style.left = l;
      }
    else
      {
      console.log( "cannot find element 'prev'" );
      }

    var nextID = document.getElementById( "next" );
    if( nextID )
      {
      var t = rect.top + (rect.bottom-rect.top)/2 - 10;
      var l = rect.right + -35;
      nextID.style.display = "block";
      nextID.style.top = t;
      nextID.style.left = l;
      }
    else
      {
      console.log( "cannot find element 'next'" );
      }

    var labelID = document.getElementById( "label" );
    if( labelID )
      {
      var t = rect.top - 40;
      var l = (rect.left + rect.right) * 0.2;
      labelID.style.display = "block";
      labelID.style.top = t;
      labelID.style.left = l;
      }
    else
      {
      console.log( "cannot find element 'label'" );
      }

    }
  }

function MyKeyDown( e )
  {
  if( e.code === "Escape" )
    {
    HideDetails();
    }
  else if( e.code === "ArrowLeft" )
    {
    PrevImage();
    }
  else if( e.code === "ArrowRight" )
    {
    NextImage();
    }
  }

function ZoomImage( camID )
  {
  /* reset tracking */
  zoomCamera = camID;
  zoomFile = "";
  zoomLive = 1;

  var imgID = document.getElementById( "photo-details" );
  if( imgID )
    {
    var url = "single-image?cam=" + camID + "\&time="
              + new Date().getTime();
    imgID.addEventListener('load', AddNavigationToDetails );
    imgID.src = url;

    var labelID = document.getElementById( "label" );
    if( labelID )
      {
      labelID.innerHTML = "<p>" + camID + " (most recent)</p>";
      labelID.style.color = "#606";
      labelID.style.fontFamily = "sans-serif";
      labelID.style.fontWeight = "bold";
      labelID.style.fontSize = "30px";
      }

    var divID = document.getElementById( "details" );
    if( divID )
      {
      divID.style.display = 'block';
      }

    document.addEventListener('keydown', MyKeyDown);
    // document.addEventListener('click', HideDetails);
    }
  else
    {
    console.log("... cannot find photo-details." );
    }
  }

function StateChangedStepImage( xhttp, whichImage, camID )
  {
  if( xhttp.readyState == 4 && xhttp.status == 200 )
    {
    var tmpName = xhttp.responseText;
    var returnData = JSON.parse( tmpName );
    if( returnData
        && returnData.images
        && returnData.images.length
        && returnData.images.length>=(whichImage+1)
        && returnData.images[0].filename )
      {
      desiredStepFile = returnData.images[whichImage].filename;
      LoadSingleSpecificImageDetailedOnly( camID, desiredStepFile );
      }
    }
  }

function PrevImage()
  {
  zoomLive = 0;
  var url = "";
  if( "" === zoomFile )
    {
    url = "single-image?cam="+zoomCamera+"\&back=1";
    }
  else
    {
    url = "single-image?cam="+zoomCamera+"\&file="+zoomFile+"\&back=1";
    }
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() { StateChangedStepImage( xhttp, 0, zoomCamera ); };
  xhttp.open( "GET", url, true );
  xhttp.send();
  }

function NextImage()
  {
  zoomLive = 0;
  var url = "";
  if( "" === zoomFile )
    {
    url = "single-image?cam="+zoomCamera+"\&forward=1";
    }
  else
    {
    url = "single-image?cam="+zoomCamera+"\&file="+zoomFile+"\&forward=1";
    }
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() { StateChangedStepImage( xhttp, 1, zoomCamera ); };
  xhttp.open( "GET", url, true );
  xhttp.send();
  }

function HideDetails()
  {
  // document.removeEventListener('click', HideDetails);
  document.removeEventListener('keydown', MyKeyDown);
  document.getElementById( 'details' ).style.display = 'none';
  document.getElementById( 'prev' ).style.display = 'none';
  document.getElementById( 'next' ).style.display = 'none';
  document.getElementById( 'label' ).style.display = 'none';
  var imgID = document.getElementById( "photo-details" );
  if( imgID )
    {
    imgID.onload = null;
    }
  }

function CamViewOnLoad()
  {
  var str = ""
  var i = 0;
  for( i=0; i < nCams; i++ )
    {
    var cam = cameras[i];
    var thisstr = "";

    thisstr = "<div style=\"position: relative; z-index:10; display: inline-block; padding: 10px 10px 10px 10px;\"><a href=\"\#\" title=\"Click to zoom in to "+cam.name+"\"";
    thisstr += " onclick=\"ZoomImage('" + cam.name + "'); return false;\">";
    thisstr += "<img style=\"border:3px solid #fff;\" id=\"img-" + cam.name + "\" src=\"single-image?cam=" + cam.name + "\&maxwidth=500\" alt=\"" + cam.name + "\"/>";
    thisstr += "</a></div>";

    str += thisstr + "\n";
    }
  var objID = document.getElementById( "cameras" );
  if( objID )
    {
    objID.innerHTML = str;
    }
  else
    {
    alert("Cannot locate cameras div tag");
    }

  window.setInterval( GetLatestAllCameras, 500 );

  /* wait for images to load before tweaking them to show which ones are ancient */
  window.setTimeout( InitialStateAllCameras, 5000 );
  }

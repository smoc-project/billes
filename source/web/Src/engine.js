// Refresh rate
let trefresh = 50;

var x=200;
var y=200;
let a=0;
let s=1;

const r = 40;

let players = [];

const svgNS="http://www.w3.org/2000/svg";

let board = document.getElementById("board");

let xreq = new XMLHttpRequest();
xreq.onreadystatechange = handle_orders;

class Player {
  constructor( rank, name, color, x, y ) {
    this.rank = rank;
	this.name = name;
    this.color = color;
	
    //this.x = x;
	//this.y = y;
	//this.rot = 0;
	//this.scale = 1;
	
	this.g = create_ball( rank, name, color, x, y );
	
	this.text = this.g.children[2];
  }
  
  set_text( txt )
  {
	this.text.innerHTML = txt;
  }
  
  set_pos( x, y, a, s )
  {
    let scale = 1;
    if ( s ) scale = s / 1000;
    let angle = 0;
    if ( a ) angle = a;

    this.g.setAttribute( "transform", "translate(" + x + "," + y + ") rotate(" + angle + ") scale(" + scale + ")" );
  }
}



function create_ball( rank, name, color, x, y )
{
  g = document.createElementNS( svgNS, 'g');

  g.setAttribute("transform", "translate(" + x + "," + y + ")" );
  g.setAttribute("id", "p_" + rank );

  c = document.createElementNS( svgNS, 'circle');
  c.setAttribute("fill", color );
  c.setAttribute("r", r );
  g.appendChild( c );
  
  c = document.createElementNS( svgNS, 'circle');
  c.setAttribute("fill", "url(#virtual_light)" );
  c.setAttribute("r", r );
  g.appendChild( c );
  
  t = document.createElementNS( svgNS, 'text');
  t.setAttribute("font-size","10");
  t.setAttribute("fill", "#000" );
  t.setAttribute("text-anchor","middle");
  t.innerHTML = name;
  g.appendChild( t );
  
  board.appendChild( g );
  
  return g;
}

function create_star( x, y, s )
{
  u = document.createElementNS( svgNS, 'use');

  u.setAttribute("href","#star");
  u.setAttribute("transform", "translate(" + x + "," + y + ") scale(" + s + ")"  );
  
  board.appendChild( u );
  
  return u;
}


function handle_orders( event )
{
  x = event.target;
  if ( !x ) return false;
  if ( ( x.readyState == 4 ) && ( x.status == 200 ) )
  {
    apply_orders( x.responseText );
  }
  return true;
}


function automate()
{
  xreq.open( "GET", "/!!" );
  xreq.send();

  if ( trefresh > 0 )
  {
    setTimeout( function() { automate(); }, trefresh ); 
  }
}

function apply_orders( jorders )
{
  if ( !jorders ) return;
				
  if ( jorders == "{}" ) return;
				
  var orders_array = JSON.parse( jorders );
  
  if ( !orders_array || !orders_array.L ) return;

  orders_array.L.forEach( function( orders ) {
	  if ( orders.move )
	  {
		orders.move.forEach( function( item ) {
	      if ( players[ item.i ] ) {
		    players[ item.i ].set_pos( item.x, item.y, item.a, item.s );
		  }
	    });
	  }
	  
	  if ( orders.new_player )
	  {
		orders.new_player.forEach( function( item ) {
	      players[ item.i ] = new Player( item.i, item.name, item.color, 0, 0 );
	    });
	  }
	  
	  if ( orders.display )
	  {
		orders.display.forEach( function( item ) {
	      display( item.id, item.x, item.y, item.scale, item.content );
	    });
	  }
  });
  
}

function display( id, x, y, s, content )
{
  t = document.getElementById(id);
	
  if ( !t )
  {
    t = document.createElementNS( svgNS, 'text');
    t.setAttribute("id",id);
    t.setAttribute("font-size","10");
    t.setAttribute("fill", "#000" );
	board.appendChild( t );
  }

  let scale = 1;
  if ( s ) scale = s;

  t.setAttribute( "transform", "translate(" + x + "," + y + ") scale(" + scale + ")" );
  //t.setAttribute("text-anchor","middle");
  if ( content ) { t.innerHTML = content; }
}

function getRandomInt(max)
{
  return Math.floor(Math.random() * Math.floor(max));
}

function do_onload( t )
{
	
  if ( t > 0 ) { trefresh = t; }

  board.setAttribute( "transform", "translate(" + r + "," + r + ")" );
  
  let n_star = 200;
  for ( i = 0 ; i < n_star ; i++ )
  {
	let sx = getRandomInt( 998 ) + 1;
	let sy = getRandomInt( 998 ) + 1;
	let sscale = i % 7 ? 1 : 1.5;
	create_star( sx, sy, sscale );
  }

// TEST
//  apply_orders(
//    '{"new_player": [ { "i": 2, "name": "Red", "color": "red" }, { "i": 1, "name": "Green", "color": "green" }, { "i": 0, "name": "Blue", "color": "blue" }, { "i": 5, "name": "Pink", "color": "pink" } ] }'
//  );
//  apply_orders(
//    '{"move": [ { "i": 1, "x": 450, "y": 550, "a": 90, "s":1.2 }, { "i": 0, "x": 550, "y": 650, "a": 90, "s":1.5 }, { "i": 5, "x": 50, "y": 800 } ] }'
//  );
//  apply_orders(
//    '{"display": [ { "id": "score", "x": 10, "y": -10, "content": "WAITING" } ] }'
//  );
//  apply_orders(
//    '{"display": [ { "id": "score", "x": 10, "y": -20, "scale": 2.5 } ] }'
//  );

  automate();
}

function register_player()
{
  let req = new XMLHttpRequest();
  //req.open( "GET", "/register_player/" + my_name );
  req.open( "GET", "/register_player" );
  req.send();
}

function test_acceleration( ax, ay ) // FOR TEST ONLY
{
  let req = new XMLHttpRequest();
  req.open( "GET", "/acc/" + ax + "/" + ay );
  req.send();
}

do_onload(100);

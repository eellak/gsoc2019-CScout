import React, { Component } from 'react';
import './App.css';
import Toolbar from './Components/Toolbar/Toolbar';
import Main from './Components/Main';
import Metrics from './Components/Metrics';
import SideDrawer from './Components/Toolbar/SideDrawer';
import Backdrop from './Components/Toolbar/Backdrop';
import FBrowse from './Components/Files/FileBrowser/FBrowse';
import Source from './Components/Files/Source/Source';
import FileSearch from './Components/Files/FileSearch/FileSearch';
class App extends Component {
  constructor() {
    super();
    this.state =  { 
      type: "homepage",
      sideDrawerOpen: false
    }
  }

  setType = (type) => {
    this.setState({
      sideDrawerOpen: false,
      type: type
    });
  };

  drawerToggleClickHandler = () => {
    this.setState((prevState) => {
      return {sideDrawerOpen: !prevState.sideDrawerOpen};
    });
  };

  backdropClickHandler = () => {
    this.setState({sideDrawerOpen: false});
  };

  linkSrc = (obj) => {
      this.setState({
        type: "link",
        id: obj
      });
  }
  render() {
    let to_render;
    let backdrop;
    if (this.state.sideDrawerOpen) {
      backdrop = <Backdrop click={this.backdropClickHandler}/>;
    }
    switch(this.state.type) {
      case("filemetrics"):
        to_render = <Metrics type='file'/>;
        break;
      case("idmetrics"):
        to_render = <Metrics type='id'/>;
        break;
      case("funmetrics"):
        to_render = <Metrics type='fun'/>;
        break;
      case("homepage"):
        to_render = <Main changeType={this.setType}/>;
        break;
      case("browse"):
        to_render = <FBrowse type='top' changeType={this.setType}/>
        break;
      case("source"):
        to_render = <Source id='15' changeType={this.linkSrc}/>
        break;
      case("link"):
        to_render = <div>{JSON.stringify(this.state)}</div>
        break;
      case("searchf"):
        to_render = <div><FileSearch/></div>
        break;
      default:
        to_render =  <h1>{this.state.type}</h1>
    }
    return (
      <div className="App">
          <SideDrawer changeType={this.setType} show={this.state.sideDrawerOpen}
            drawerClickHandler={this.drawerToggleClickHandler}/>
          {backdrop}
          <Toolbar changeType={this.setType} style={{zIndex:'20'}}  
            drawerClickHandler={this.drawerToggleClickHandler} style={{zIndex:"600"}}/>
          {to_render}
      </div>
    );
  }
}

export default App;

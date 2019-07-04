import React, { Component } from 'react';
import './App.css';
import Toolbar from './Components/Toolbar/Toolbar';
import Main from './Components/Main';
import Filemetrics from './Components/Filemetrics';
import SideDrawer from './Components/Toolbar/SideDrawer';
import Backdrop from './Components/Toolbar/Backdrop';
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
      type: type,
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

  render() {
    let to_render;
    let backdrop;
    if (this.state.sideDrawerOpen) {
      backdrop = <Backdrop click={this.backdropClickHandler}/>;
    }
    switch(this.state.type) {
      case("files"):
        to_render = <Filemetrics/>;
        break;
      case("homepage"):
        to_render = <Main/>;
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

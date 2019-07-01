import React, { Component } from 'react';
import './App.css';
import Toolbar from './Components/Toolbar/Toolbar'
import Filemetrics from './Components/Filemetrics'
class App extends Component {
  constructor() {
    super();
    this.state =  { 
      type: "homepage"
    }
  }

  setType = (type) => {
    this.setState({
      type: type,
    });
  };


  render() {
    let to_render;
    switch(this.state.type) {
      case("files"):
        to_render = <Filemetrics/>;
        break;
      default:
        to_render =  <h1>{this.state.type}</h1>
    }
    return (
      <div className="App">
          <Toolbar changeType={this.setType} style={{zIndex:'20'}} 
            style={{zIndex:"600"}}/>
          {to_render}
      </div>
    );
  }
}

export default App;

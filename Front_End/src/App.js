import React, { Component } from 'react';
import logo from './logo.svg';
import './App.css';
import Toolbar from './Components/Toolbar'

class App extends Component {
  constructor(){
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
    return (
      <div className="App">
          <Toolbar changeType={this.setType} style={{zIndex:'20'}} 
            style={{zIndex:"600"}}/>
          <h1>{this.state.type}</h1>
      </div>
    );
  }
}

export default App;

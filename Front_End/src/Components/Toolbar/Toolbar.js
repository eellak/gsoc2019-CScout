import React,{Component} from 'react';
import './Toolbar.css';
import logo from '../../logo.png';
import DrawerToggleButton from './DrawerToggleButton';
import Popup from 'reactjs-popup';
import Refactorings from '../Refactorings';
import Replacements from '../Replacements';
import SelectProj from '../SelectProj';
import Axios from 'axios';

class Toolbar extends Component{ 
    constructor(props) {
        super(props);
        this.state = { open: false, popUp: null };
        this.openModal = this.openModal.bind(this);
        this.closeModal = this.closeModal.bind(this);
      }

      openModal(toRender) {
        this.setState({ open: true, popUp: toRender });
      }

      closeModal() {
        this.setState({ open: false });
      }

      render(){
        return( 
        <header className="toolbar">
            <nav className="toolbar_navigation">
                <div className="toolbar_toggle-button"><DrawerToggleButton click={this.props.drawerClickHandler} /></div>
                <div className="toolbar_logo" onClick={() => this.props.changeType("homepage")} style={{ cursor: 'pointer' }}>
                    <a href="home"><img src={logo} alt="C" /></a>
                    <h1>Scout</h1>
                </div>
                <div className="spacer"></div>
                <div className="toolbar_navigation-items">
                    <ul>
                        <li>
                            <a style={{ top: '8px', bottom: '8px' }} onClick={() => this.openModal(
                                <Replacements changeType={this.props.changeType} closeModal={this.closeModal} openModal={this.openModal}/>
                            )}>
                                Identifier<br />replacements</a>
                        </li>
                        
                        <li>
                            <a style={{ top: '8px', bottom: '8px' }} onClick={() => this.openModal(
                                <Refactorings changeType={this.props.changeType} closeModal={this.closeModal} openModal={this.openModal}/>
                            )}>
                                Function<br />refactorings</a>
                        </li>

                        <li>
                           <a  style={{ top: '8px', bottom: '8px' }} onClick={() => this.openModal(
                             <SelectProj/>
                           )}>Select<br />project</a>                          
                        </li>
                        
                        <li>
                            <a onClick={() => this.openModal(<div id="exit">
                                    <div>Do you want to save changes?</div>
                                    <div>
                                        <button onClick={() => {
                                            Axios.put(global.address + "sexit.html")
                                            .then((response) =>  this.openModal(
                                                response.data.exit?<div> Saved and Exited<br/>{response.data.statistics.msg}</div>
                                                :<div>Save and Exit Failed:{response.data.error}</div>)
                                            ) 
                                        }}
                                        className="formButton" >Yes</button><div style={{display:"inline"}}>{"   "}</div>
                                        <button onClick={() => Axios.get(global.address + "qexit.html")} className="formButton">No</button><div style={{display:"inline"}}>{"   "}</div>
                                        <button onClick={this.closeModal} className="formButton">
                                            Cancel
                                        </button>
                                    </div>
                                </div>)}>
                                   Exit</a>
                          
                        </li>
                    </ul>
                    <Popup open={this.state.open} modal closeOnDocumentClick closeOnEscape onClose={this.closeModal}>
                                       <div> {this.state.popUp}</div>
                    </Popup>
                </div>
            </nav>
        </header>
        )
    }
}

export default Toolbar;
import React, { Component } from 'react';
import './SideDrawer.css';
import Popup from 'reactjs-popup';


class SideDrawer extends Component {
    constructor(props) {
        super(props);
        this.state = {
            query: 0,
            drawerClasses: (props.show) ? "side-drawer open" : "side-drawer",
       		open: false, 
			popUp: null 
		};
        this.openModal = this.openModal.bind(this);
        this.closeModal = this.closeModal.bind(this);
		this.temport = 13780;
      }

      openModal(toRender) {
        this.setState({ open: true, popUp: toRender });
      }

      closeModal() {
        this.setState({ open: false });
      }


    componentDidUpdate(prevProps) {
        if (prevProps !== this.props) {
            this.setState({
                query: (this.props.show) ? this.state.query : 0,
                drawerClasses: (this.props.show) ? "side-drawer open" : "side-drawer"
            })
        }
    }

    changeType(ty) {
        this.setState({
            query: ty,
            drawerClasses: this.state.drawerClasses
        })
    };

    render() {
        return (
            <div>
                <nav className={this.state.drawerClasses}>
                    <a onClick={this.props.drawerClickHandler} id="slideBack">&#10005;</a>
                    <ul>
                        <li className='title'><b onClick={() => this.props.changeType("files")} >
                            Files</b></li>
                        <li onClick={() => this.props.changeType("browse")} className='menuOpt'>
                            <a>Browse Files</a></li>
                        <li onClick={() => this.props.changeType("searchf")} className='menuOpt'>
                            <a>Search File</a></li>
                        <li onClick={() => this.props.changeType("filemetrics")} className='menuOpt'>
                            <a>File Metrics</a></li>

                        <li className='title'><b onClick={() => this.props.changeType("identifiers")} className='title'>
                            Identifiers</b></li>
                        <li onClick={() => this.props.changeType("searchId")} className='menuOpt'>
                            <a>Search Identifier</a></li>
                        <li onClick={() => this.props.changeType("idmetrics")} className='menuOpt'>
                            <a>Identifier Metrics</a></li>

                        <li className='title'><b onClick={() => console.log("funcAndMac")} className='title'>
                            Functions and Macros</b></li>
                        <li onClick={() => this.props.changeType("searchfun")} className='menuOpt'>
                            <a>Search Functions</a></li>
                        <li onClick={() => this.props.changeType("funmetrics")} className='menuOpt'>
                            <a>Function Metrics</a></li>


                        <li className='title'><a onClick={() => this.openModal(
							<div>
								Change Connection port with Backend: <input type="number" onChange={(e) => this.tempPort = e.target.value}  style={{width:'50%'}}/><br />
								<button onClick={() => {global.address = "http://localhost:" + this.tempPort + "/";this.closeModal()}} className="formButton">OK</button>
							</div>
						)}>Options</a></li>
						 <Popup open={this.state.open} modal closeOnDocumentClick closeOnEscape onClose={this.closeModal}>
                                       <div> {this.state.popUp}</div>
                    </Popup>
                    </ul>
                </nav>
            </div>
        );
    }
}
export default SideDrawer;

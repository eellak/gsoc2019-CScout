import React ,{Component}from 'react';
import './SideDrawer.css';

class SideDrawer extends Component  {
    constructor(props){
        super(props);
        this.state = {
            query : 0,
            drawerClasses : (props.show)?"side-drawer open":"side-drawer"
        }

    }
    componentDidUpdate(prevProps) {
        if(prevProps!==this.props){
            this.setState({
                query : (this.props.show)?this.state.query:0,
                drawerClasses : (this.props.show)?"side-drawer open":"side-drawer"
            })
        }
    }
    changeType(ty){
        this.setState({
            query : ty,
            drawerClasses : this.state.drawerClasses
        })
    };
   

    render(){
    
       
        return(
        <div>
            <nav className={this.state.drawerClasses}>
            <a onClick={this.props.drawerClickHandler} id="slideBack">&#10005;</a> 
        
                
                <ul>
                    <li className='title'><b onClick={()=> this.props.changeType("files")} >
                        Files</b></li>
                    <li onClick={()=> this.props.changeType("browse")} className='menuOpt'>
                        <a>Browse Files</a></li>
                    <li onClick={()=> this.props.changeType("searchf")} className='menuOpt'>
                        <a>Search File</a></li>
                    <li onClick={()=> this.props.changeType("filemetrics")} className='menuOpt'>
                        <a>File Metrics</a></li>
                    
                    <li className='title'><b onClick={()=> this.props.changeType("identifiers")} className='title'>
                        Identifiers</b></li>
                    <li onClick={()=> this.props.changeType("brIdentifiers")} className='menuOpt'>
                        <a>Browse identifiers</a></li>
                    <li onClick={()=> this.props.changeType("searchId")} className='menuOpt'>
                        <a>Search Identifier</a></li>
                    <li onClick={()=> this.props.changeType("idmetrics")} className='menuOpt'>
                        <a>Identifier Metrics</a></li>

                    <li className='title'><b onClick={()=> console.log("funcAndMac")} className='title'>
                        Functions and Macros</b></li>
                    <li onClick={()=> this.props.changeType("brFunctions")} className='menuOpt'>
                        <a>Browse Functions</a></li>
                    <li onClick={()=> this.props.changeType("searchfun")} className='menuOpt'>
                        <a>Search Functions</a></li>
                    <li onClick={()=> this.props.changeType("funmetrics")} className='menuOpt'>
                        <a>Function Metrics</a></li>

            
                    <li className='title'><a onClick ={() => console.log('options')}>Options</a></li>

                </ul>
            </nav>
        </div>
        );
    }
}
export default SideDrawer;
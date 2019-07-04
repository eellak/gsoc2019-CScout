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
    
       
        return(<div>
        <nav className={this.state.drawerClasses}>
        <a onClick={this.props.drawerClickHandler} id="slideBack">&#10094;</a> 
            <ul>
                <li><a onClick ={() => console.log('options')}>Options</a></li>

                </ul>
            </nav>
        </div>
        );
    }
}
export default SideDrawer;